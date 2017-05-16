#
# Run as:
# $ gnuplot -e "filename='output.log'" -p -c basic.plot
#

set key off
set multiplot layout 2,2
set yrange [0:30000000]

set title "UL"
plot filename using 1:2 with lines

set title "UR"
plot filename using 1:3 with lines

set title "LL"
plot filename using 1:4 with lines

set title "LR"
plot filename using 1:5 with lines

unset multiplot
