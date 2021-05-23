import os
import subprocess, sys
import re

TESTER = '/home/kura/Documents/atcoder/AHC003/tools/target/release/tester'
TEST_DIR = '/home/kura/Documents/atcoder/AHC003/tools/in'
ANSWER = '/home/kura/Documents/atcoder/AHC003/answer'

TEST = [0, 20, 40, 60, 80]

def test(test_num):
    cmd = TESTER 
    arg1 = os.path.join(TEST_DIR, '{:04}.txt'.format(test_num))
    arg2 = ANSWER
    #print(cmd)
    cp = subprocess.run([cmd, arg1, arg2], encoding='utf-8', stderr=subprocess.PIPE, stdout=subprocess.DEVNULL)
    if cp.returncode != 0:
        print('cmd failed.', file=sys.stderr)
        sys.exit(1)
    return cp.stderr

def main():
    total = 0
    for t in TEST:
        output = test(t)
        #print(output, end='')
        score = int(re.sub(r"\D", "", output))
        print(f'test {t} : {score}')
        total += score
    print()
    print(f'total score : {total}.')
    print(f'average score : {total // len(TEST)}.')

if __name__ == '__main__':
    main()
