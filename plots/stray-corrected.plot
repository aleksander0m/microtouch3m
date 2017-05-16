#
# Run as:
# $ gnuplot -e "filename='output.log'" -p -c stray-corrected.plot
#

set key off
set multiplot layout 2,2
set yrange [0:15000000]

set title "UL(c)"
plot filename using 1:10 with lines

set title "UR(c)"
plot filename using 1:11 with lines

set title "LL(c)"
plot filename using 1:12 with lines

set title "LR(c)"
plot filename using 1:13 with lines

unset multiplot
