make clean
make project
./project --cycles 100000000 test/general_test.csv --tf "tracepath/trace" --num-cache-levels 3 --mapping-strategy 1 --cacheline-size 8 --num-lines-l1 2 --num-lines-l2 4 --num-lines-l3 8