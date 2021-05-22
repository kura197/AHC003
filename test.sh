#/bin/sh

TESTER=tools/target/release/tester
DIR_IN=tools/in
ANSWER=./answer

run_test(){
   id=`printf %04d $1`
   #echo $id
   $TESTER $DIR_IN/$id.txt $ANSWER >> /dev/null
}

run_test 0
run_test 10
run_test 20
run_test 30
run_test 40
