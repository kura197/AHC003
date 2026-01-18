import os
import subprocess, sys
import multiprocessing as mp
import numpy as np
import re
import math
import pdb
import argparse
import json
import datetime
from dataclasses import dataclass, field
from typing import List, Optional

from cloud_test import upload_solver, run_lambda_test, download_result

#TEST = [0]
#TEST = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90]
#TEST = [0, 30, 40, 50, 55, 65, 70, 75]
#TEST = [0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95]
TEST = range(0, 20)

EPS = 0.00001

## TODO: update for each contest
class Params:
    bins = {}
    ranges = {}
    #bins = {'K': 4, 'T': 15, 'D': 10}
    #ranges = {  # [left, right)
    #    'K': [4, 20+1],
    #    'T': [4000, 64000+1],
    #    'D': [10, 10000+1],
    #}

    def read_params(t, tests_path):
        param_dicts = {}
        input_file = os.path.join(tests_path, '{:04}.txt'.format(t))
        with open(input_file, 'r') as fp:
            line = fp.readline().split(' ')
            #param_dicts['K'] = int(line[1])
            #param_dicts['T'] = int(line[3])
            #param_dicts['D'] = int(line[4])
        return param_dicts

@dataclass
class RunConfig:
    """実行設定を管理するクラス"""
    tester: Optional[str] = None
    vis: str = './tools/target/release/vis'
    tests: str = './tools/in'
    solver: str = './answer'
    max_process: int = 4
    n_judge_testcase: int = 150
    cloud: bool = False
    save: str = 'last_trial'
    compare: str = 'last_trial'
    show_comparison: List[str] = field(default_factory=list)


def run_test(queue, input_file, tmp_file, test_num, params, config: RunConfig):
    str_params = [str(x) for x in params]

    if config.tester is not None:  ## interactive
        with open(tmp_file, 'w') as ofp:
            proc = subprocess.run([config.tester, input_file, config.solver, *str_params], encoding='utf-8', stderr=subprocess.PIPE, stdout=ofp)
        if proc.returncode != 0:
            print(f'{config.tester} {config.solver} failed.', file=sys.stderr)
            print(proc.stderr)
            sys.exit(1)
        score = int(re.sub(r"\D", "", proc.stderr[-10:]))
    else:
        ## TODO: test
        with open(input_file, 'r') as ifp:
            with open(tmp_file, 'w') as ofp:
                proc = subprocess.run([config.solver, *str_params], encoding='utf-8', stderr=subprocess.PIPE, stdout=ofp, stdin=ifp)
        if proc.returncode != 0:
            print(f'{config.solver} failed.', file=sys.stderr)
            print(proc.stderr)
            sys.exit(1)

        proc = subprocess.run([config.vis, input_file, tmp_file], encoding='utf-8', stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        if proc.returncode != 0:
            print(f'{config.vis} failed.', file=sys.stderr)
            print(proc.stderr)
            sys.exit(1)
        score = int(re.sub(r"\D", "", proc.stdout[-10:]))
    queue.put(score)

def load_base_scores(compare_target_name):
    print(f"INFO: Attempting to load base scores from './out/{compare_target_name}'")
    summary_path = os.path.join('out', compare_target_name, 'summary.json')

    if not os.path.exists(summary_path):
        print("INFO: Comparison target not found. Using default base scores (1).")
        return None

    try:
        with open(summary_path, 'r') as f:
            data = json.load(f)
        scores_dict = {
            item['test_case']: item['score']
            for item in data['individual_test_scores']
        }
        print(f"INFO: Successfully loaded {len(scores_dict)} base scores for comparison.")
        return scores_dict
    except (json.JSONDecodeError, KeyError) as e:
        print(f"WARN: Error reading or parsing {summary_path}: {e}. Using default base scores (1).")
        return None

def run_lambda_test_async(queue, test_case, eval_type, params):
    score = run_lambda_test(test_case, eval_type, params)
    queue.put(score)

def run_process(test_num, params, config: RunConfig):
    if config.cloud:
        eval_type = 'tester' if config.tester is not None else 'vis'
        queue = mp.Queue()
        proc = mp.Process(target=run_lambda_test_async, args=(queue, test_num, eval_type, params))
        proc.start()
    else:
        output_dir = os.path.join('out', config.save)
        input_file = os.path.join(config.tests, '{:04}.txt'.format(test_num))
        tmp_file = os.path.join(output_dir, '{:04}.txt'.format(test_num))

        queue = mp.Queue()
        proc = mp.Process(target=run_test, args=(queue, input_file, tmp_file, test_num, params, config))
        proc.start()
    return proc, queue

def proc_wait(test_num, proc, queue):
    proc.join()
    score = queue.get()
    return score

class Result:
    def __init__(self, config: RunConfig, base_scores={}, comparison_scores={}, trials_to_show_order=[]):
        self.config = config
        self.base_scores = base_scores
        self.comparison_scores = comparison_scores
        self.trials_to_show_order = trials_to_show_order
        self.total = 0
        self.ignore_rank = 2
        self.rel_result = []
        self.param_scores = []
        self.file_out = ''
        self.dec_case = 0
        self.inc_case = 0
        self.individual_scores = []
        self.header_printed = False
        self.score_col_width = 20
        self.param_col_width = 8

    def _print_header(self):
        header_line = f"{'test':>4} |"
        for name in Params.bins.keys():
            header_line += f" {name:>{self.param_col_width}} |"
        header_line += f" {self.config.save:<{self.score_col_width}} |"
        for name in self.trials_to_show_order:
            header_line += f" {name:<{self.score_col_width}} |"
        print(header_line)
        print('-' * len(header_line))

    def write_file(self, filename):
        with open(filename, 'w') as f:
            f.write(self.file_out)

    def get_ave_rel_total(self):
        self.rel_result.sort()
        sum_val = sum(self.rel_result[self.ignore_rank:-self.ignore_rank])
        sz = len(self.rel_result) - 2*self.ignore_rank
        return 2**(sum_val / sz)

    def add_score(self, t, score, show_score=True):
        self.total += score
        self.file_out += 'BASE_SCORE[{}] = {}\n'.format(t, score)

        rel_base = self.base_scores.get(t, 1)
        rel = 0 if rel_base == 0 else 100 * score / rel_base
        self.rel_result.append(math.log2(rel/100 + 1e-20))
        if score < (rel_base - 1e-10):
            self.dec_case += 1
        elif score > (rel_base + 1e-10):
            self.inc_case += 1

        params = Params.read_params(t, self.config.tests)
        self.individual_scores.append({
            'test_case': t,
            'score': score,
            'base_score': rel_base,
            'relative_score_percent': rel,
            'params': params
        })
        self.param_scores.append((params, rel / 100.0))

        if show_score:
            if not self.header_printed:
                self._print_header()
                self.header_printed = True

            output_line = f"{t:04d} |"
            for name in Params.bins.keys():
                output_line += f" {params.get(name, ''):>{self.param_col_width}} |"
            
            current_rel = 0 if rel_base == 0 else (score / rel_base) * 100
            cell_str_current = f"{score:,d} ({current_rel:6.2f}%)"
            output_line += f" {cell_str_current:>{self.score_col_width}} |"
            
            for trial_name in self.trials_to_show_order:
                past_score_dict = self.comparison_scores.get(trial_name)
                cell_str_past = ""
                if past_score_dict:
                    past_score = past_score_dict.get(t)
                    if isinstance(past_score, (int, float)):
                        past_rel = 0 if rel_base == 0 else (past_score / rel_base) * 100
                        cell_str_past = f"{past_score:,d} ({past_rel:6.2f}%)"
                    else:
                        cell_str_past = "---"
                else:
                    cell_str_past = "N/A"
                output_line += f" {cell_str_past:>{self.score_col_width}} |"
            print(output_line)

def load_all_comparison_scores(trial_names):
    all_scores = {}
    print(f"INFO: Loading data for side-by-side comparison: {trial_names}")
    for name in trial_names:
        summary_path = os.path.join('out', name, 'summary.json')
        if not os.path.exists(summary_path):
            continue
        try:
            with open(summary_path, 'r') as f:
                data = json.load(f)
            scores_dict = {item['test_case']: item['score'] for item in data['individual_test_scores']}
            all_scores[name] = scores_dict
        except Exception as e:
            print(f"WARN: Could not load summary for '{name}': {e}. Skipping.")
    return all_scores

def get_score(config: RunConfig, show_score=False, params=[], base_scores={}, comparison_scores={}, trials_to_show_order=[]):
    output_dir = os.path.join('out', config.save)
    os.makedirs(output_dir, exist_ok=True)
    print(f"Results will be saved to: {output_dir}")

    print('params = {}'.format(params))
    result = Result(
        config,
        base_scores=base_scores, 
        comparison_scores=comparison_scores, 
        trials_to_show_order=trials_to_show_order
    )
    proc_list = []

    max_parallel = 100 if config.cloud else config.max_process  ## TODO: check
    
    for t in TEST:
        proc, queue = run_process(t, params, config)
        proc_list.append((t, proc, queue))
        if len(proc_list) == max_parallel:
            t, proc, queue = proc_list.pop(0)
            score = proc_wait(t, proc, queue)
            result.add_score(t, score, show_score=show_score)

    for t, proc, queue, in proc_list:
        score = proc_wait(t, proc, queue)
        result.add_score(t, score, show_score=show_score)

    ave_score = result.total / len(TEST) if len(TEST) > 0 else 0
    if not show_score:
        #return ave_score
        return result.get_ave_rel_total()

    print('Absolute')
    print('total score : {:,}.'.format(result.total))
    print('average score : {:,.3f}.'.format(ave_score))
    print('estimated score : {:,}.'.format(ave_score * config.n_judge_testcase))

    print('Relative')
    print('average score diff ratio: {:+.4f} %.'.format(result.get_ave_rel_total() * 100 - 100))
    print('num of score increase : {:2}.'.format(result.inc_case))
    print('num of score decrease : {:2}.'.format(result.dec_case))

    host = 'cloud' if config.cloud else 'local'
    summary_data = {
        'absolute_scores': {
            'total': result.total,
            'average': ave_score,
            'estimated_total': ave_score * config.n_judge_testcase,
        },
        'relative_scores': {
            'average_log_ratio': result.get_ave_rel_total(),
            'improved_cases': result.inc_case,
            'decreased_cases': result.dec_case,
        },
        'parameter_analysis': {},
        'individual_test_scores': result.individual_scores,
        'host': host
    }

    for name in Params.bins:
        summary_data['parameter_analysis'][name] = []
        tot_scores = [0.0 for _ in range(Params.bins[name])]
        hist = [0 for _ in range(Params.bins[name])]
        width = ((Params.ranges[name][1] - Params.ranges[name][0]) + Params.bins[name] - 1) // Params.bins[name]
        for (params, score) in result.param_scores:
            val = params[name]
            idx = (val - Params.ranges[name][0]) // width
            tot_scores[idx] += score - 1
            hist[idx] += 1
        print(name)
        for i in range(Params.bins[name]):
            ave_score_ratio = tot_scores[i]/hist[i] if hist[i] > 0 else 0.0
            range_str = '[{} - {})'.format(Params.ranges[name][0] + width*i, Params.ranges[name][0] + width*(i+1))
            print('{} : {:.6f} ({})'.format(range_str, ave_score_ratio, hist[i]))
            summary_data['parameter_analysis'][name].append({
                'range': range_str,
                'average_score_change': ave_score_ratio,
                'count': hist[i]
            })

    result.write_file('base_score.txt')

    summary_data['timestamp'] = datetime.datetime.now().isoformat()
    summary_filepath = os.path.join(output_dir, 'summary.json')
    with open(summary_filepath, 'w') as f:
        json.dump(summary_data, f, indent=2)
    print(f"\nScore summary saved to {summary_filepath}")

    #return ave_score
    return result.get_ave_rel_total()

def main(args):
    save_filename = args.save if args.save is not None else datetime.datetime.now().strftime('%Y-%m-%d_%H-%M-%S')
    # argparseの結果をRunConfigオブジェクトに変換
    config = RunConfig(
        tester=args.tester,
        vis=args.vis,
        tests=args.tests,
        solver=args.solver,
        max_process=args.max_process,
        n_judge_testcase=args.n_judge_testcase,
        cloud=args.cloud,
        save=save_filename,
    )

    loaded_scores = load_base_scores(args.compare)
    base_scores_for_run = loaded_scores if loaded_scores else {}

    trials_to_show_order = args.show_comparison
    all_past_scores = load_all_comparison_scores(trials_to_show_order)

    if config.cloud:
        upload_solver()
    else:
        proc = subprocess.run(['make', 'answer'], encoding='utf-8')
        if proc.returncode != 0:
            sys.exit(1)
            
    get_score(
        config,
        show_score=True,
        params=[],
        base_scores=base_scores_for_run,
        comparison_scores=all_past_scores,
        trials_to_show_order=trials_to_show_order
    )

def get_argparser():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--tester', type=str, default='./tools/target/release/tester', help='tester path')
    parser.add_argument('--vis', type=str, default=None, help='vis path')
    parser.add_argument('--tests', type=str, default='./tools/in', help='input test case path')
    parser.add_argument('--solver', type=str, default='./answer', help='solver bin path')
    parser.add_argument('--max_process', type=int, default=4, help='max number of process')
    parser.add_argument('--n_judge_testcase', type=int, default=100, help='number of testcase in judge system')
    parser.add_argument('--cloud', action='store_true', help='run tests at cloud server')
    parser.add_argument('--save', type=str, default=None, help='directory name to save results under ./out/')
    parser.add_argument('--compare', type=str, default='solve08', help='directory name under ./out/ to use as a baseline for comparison')
    parser.add_argument('--show-comparison', type=str, nargs='*', default=['solve08', 'old_solver', 'eivour'], help='List of additional past trials to show in the final comparison table.')
    return parser

if __name__ == '__main__':
    args = get_argparser().parse_args()
    main(args)
