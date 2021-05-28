import os
import subprocess, sys
import re
import pdb
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--init_val', default=3000, type=int, help='[TEST] initial value')
parser.add_argument('--bsize', default=5, type=int, help='[TEST] batch size')

TESTER = '/home/kura/Documents/atcoder/AHC003/tools/target/release/tester'
TEST_DIR = '/home/kura/Documents/atcoder/AHC003/tools/in'
ANSWER = '/home/kura/Documents/atcoder/AHC003/answer'

#TEST = [0]
#TEST = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90]
#TEST = [0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95]
TEST = [0, 2, 5, 8, 10, 12, 15, 18, 20, 22, 25, 28, 30, 32, 35, 38, 40, 42, 45, 48, 
        50, 52, 55, 58, 60, 62, 65, 68, 70, 72, 75, 78, 80, 82, 85, 88, 90, 92, 95, 98]

MAX_P = 10

def run_process(test_num, args):
    #cmd = TESTER 
    arg1 = os.path.join(TEST_DIR, '{:04}.txt'.format(test_num))
    arg2 = ANSWER
    #cmd = [TESTER, arg1, arg2]
    cmd = [TESTER, arg1, arg2, str(args.init_val), str(args.bsize)]
    #print(cmd + ' ' + arg1 + ' ' + arg2)
    proc = subprocess.Popen(cmd, encoding='utf-8', stderr=subprocess.PIPE, stdout=subprocess.DEVNULL)
    return proc

def proc_wait(proc):
    proc.wait()
    if proc.returncode != 0:
        print('cmd failed.', file=sys.stderr)
        sys.exit(1)
    #print(proc.communicate()[1])
    score = int(re.sub(r"\D", "", proc.communicate()[-1]))
    return score

def main(args):
    total = 0
    proc_list = []

    for t in TEST:
        proc = run_process(t, args)
        proc_list.append((t, proc))
        if len(proc_list) == MAX_P:
            t, proc = proc_list.pop(0)
            score = proc_wait(proc)
            print('test {:02} : {}'.format(t, score))
            total += score

    for t, proc in proc_list:
        score = proc_wait(proc)
        print('test {:02} : {}'.format(t, score))
        total += score

    print()
    print(f'total score : {total}.')
    print(f'average score : {total // len(TEST)}.')

if __name__ == '__main__':
    args = parser.parse_args()
    main(args)
