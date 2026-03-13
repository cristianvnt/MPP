#!/bin/bash
set -e

RUNS=${1:-10}
LABEL=${2:-"unnamed"}
LOGFILE=${3:-"data/benchmark.log"}
shift 3 || true
CMD="$@"

if [ -z "$CMD" ]; then
    echo "Use: ./benchmark.sh <runs> <label> <logfile> <command...>"
    exit 1
fi

echo "Running: $CMD"
echo "Runs: $RUNS"

t_read_sum=0
t_mult_sum=0
t_write_sum=0
t_total_sum=0

for i in $(seq 1 $RUNS); do
    output=$($CMD)
    t_read=$(echo "$output" | grep t_reading | awk '{print $2}')
    t_mult=$(echo "$output" | grep t_multiply | awk '{print $2}')
    t_write=$(echo "$output" | grep t_writing | awk '{print $2}')
    t_total=$(echo "$output" | grep t_total | awk '{print $2}')

    echo ">> Run $i: read=$t_read mult=$t_mult write=$t_write total=$t_total"

    t_read_sum=$(echo "$t_read_sum + $t_read" | bc)
    t_mult_sum=$(echo "$t_mult_sum + $t_mult" | bc)
    t_write_sum=$(echo "$t_write_sum + $t_write" | bc)
    t_total_sum=$(echo "$t_total_sum + $t_total" | bc)
done

t_read_avg=$(echo "scale=7; $t_read_sum / $RUNS" | bc)
t_mult_avg=$(echo "scale=7; $t_mult_sum / $RUNS" | bc)
t_write_avg=$(echo "scale=7; $t_write_sum / $RUNS" | bc)
t_total_avg=$(echo "scale=7; $t_total_sum / $RUNS" | bc)

echo ""
echo "Averages over $RUNS runs:"
echo ">> t_reading: $t_read_avg ms"
echo ">> t_multiply: $t_mult_avg ms"
echo ">> t_writing: $t_write_avg ms"
echo ">> t_total: $t_total_avg ms"

echo "[$LABEL] $(date '+%Y-%m-%d %H:%M') | runs=$RUNS | read=$t_read_avg | mult=$t_mult_avg | write=$t_write_avg | total=$t_total_avg ms" >> "$LOGFILE"
echo "Logged to $LOGFILE"