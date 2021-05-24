import os
import subprocess, sys
import re
import pdb

TESTER = '/home/kura/Documents/atcoder/AHC003/tools/target/release/tester'
TEST_DIR = '/home/kura/Documents/atcoder/AHC003/tools/in'
ANSWER = '/home/kura/Documents/atcoder/AHC003/answer'

#TEST = [0, 10, 20, 30, 40, 50, 60, 70, 80, 90]
TEST = [0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95]

def run_process(test_num):
    cmd = TESTER 
    arg1 = os.path.join(TEST_DIR, '{:04}.txt'.format(test_num))
    arg2 = ANSWER
    #print(cmd)
    #proc = subprocess.run([cmd, arg1, arg2], encoding='utf-8', stderr=subprocess.PIPE, stdout=subprocess.DEVNULL)
    proc = subprocess.Popen([cmd, arg1, arg2], encoding='utf-8', stderr=subprocess.PIPE, stdout=subprocess.DEVNULL)
    return proc

def main():
    total = 0
    proc_list = []

    for t in TEST:
        proc = run_process(t)
        proc_list.append((t, proc))

    for t, proc in proc_list:
        proc.wait()
        if proc.returncode != 0:
            print('cmd failed.', file=sys.stderr)
            sys.exit(1)
        #print(proc.communicate()[1])
        score = int(re.sub(r"\D", "", proc.communicate()[-1]))
        print(f'test {t} : {score}')
        total += score

    print()
    print(f'total score : {total}.')
    print(f'average score : {total // len(TEST)}.')

if __name__ == '__main__':
    main()
