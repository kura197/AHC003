import sys
import optuna
import subprocess
import re
import pdb

def run_process(init_val, field_var, naive_epoch, ker_ratio, ker_len):
    #cmd = CMD 
    #print(cmd + ' ' + arg1 + ' ' + arg2)
    cmd = ['python3', 'test.py', '--init_val', str(init_val), '--field_var', str(field_var), '--naive_epoch', str(naive_epoch), 
            '--ker_ratio', str(ker_ratio), '--ker_len', str(ker_len)]
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
    init_val = trial.suggest_int('init_val', 1000, 8000) 
    field_var = trial.suggest_int('field_var', 0, 5000) 
    naive_epoch = trial.suggest_int('naive_epoch', 0, 400) 
    ker_ratio = trial.suggest_int('ker_ratio', 0.5, 9.5) 
    ker_len = trial.suggest_int('ker_len', 1, 30) 
    score = run_process(init_val, field_var, naive_epoch, ker_ratio, ker_len)
    return -score

def main():
    study = optuna.create_study(pruner=optuna.pruners.HyperbandPruner())
    print(f"Sampler is {study.sampler.__class__.__name__}")
    study.optimize(objective, n_trials=100)
    #run_process(1000)

if __name__ == '__main__':
    main()
