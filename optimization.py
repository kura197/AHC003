import sys
import optuna
import subprocess
import re
import pdb

def run_process(init_val, bsize):
    #cmd = CMD 
    #print(cmd + ' ' + arg1 + ' ' + arg2)
    cmd = ['python3', 'test.py', '--init_val', str(init_val), '--bsize', str(bsize)]
    #cmd = ['python3', 'test.py']
    #print(cmd)
    proc = subprocess.run(cmd, encoding='utf-8', stderr=subprocess.DEVNULL, stdout=subprocess.PIPE)
    if proc.returncode != 0:
        print('cmd failed.', file=sys.stderr)
        sys.exit(1)
    ### extrace only last value
    score_str = re.split(':', proc.stdout)[-1]
    score = int(re.sub(r"\D", "", score_str))
    return score 

def objective(trial):
    #x = trial.suggest_uniform('x', -10, 10) 
    #score = (x - 2) ** 2;
    #print('x: {:1.3}, score: {:1.3}'.format(x, score))
    #init_val = trial.suggest_uniform('init_val', 100, 10000) 
    init_val = trial.suggest_int('init_val', 100, 10000) 
    bsize = trial.suggest_int('bsize', 1, 50) 
    score = run_process(init_val, bsize)
    return -score

def main():
    study = optuna.create_study()
    study.optimize(objective, n_trials=10)
    #run_process(1000)

if __name__ == '__main__':
    main()
