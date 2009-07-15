#/bin/sh
graphs="1 2 3 4"
procs="1 8"
time=1

# $B!&@-G=I>2A$H$OJL$K!$(Bsearch
#      (= list (search1 v0))
#      (do-while (no-empty list) 
#         (= mylist list) (= list nil)
#         (for each v in mylist (+= list (search v))))
#   $B$H9M$($?$H$-$N(B
#    * do-while $B$N7+$jJV$72s?t(B
#    * $B3F2s$N(B list $BD9(B
#    * $B3F2s$G(B visit $B:Q$ND:E@?t(B ($BA4D:E@$rAv::$9$l$P$h$$$G$9!%(B
#          $B%-%c%C%7%e$N>uBV$K1F6A$OM?$($=$&$G$9$,;EJ}$J$$$G$9!%(B
#          $B$A$J$_$K!$(BPARALLEL_SEARCH3 $B$G$O!$J#?t$N(Bworker $B$,0l$D$ND:E@$r(B
#          get$B$7$?$D$b$j$,!$$"$k(Bworker$B$N=q$-9~$_$N$_$,$&$^$/;D$k$H$$$&$N(B
#          $B$,$"$j$^$9$N$G!$$=$NItJ,$G$O@53N$K%+%&%s%H$G$-$^$;$s(B)
#   $B$rB,Dj$7$F$b$i$($J$$$G$7$g$&$+(B?
#   $B$3$&$7$F$_$k$H(B do-while $B$G$O$J$/(B while $B$H$9$Y$-$G$9$M!%(B

for g in $graphs; do
    for p in $procs; do
#   *  Cilk_cas
#   *  Cilk_membar
        
#   *  Tascell_cas    (list->array $B$O$I$&$7$^$7$g$&$+(B....)
#        ntimes $time ./spanning-lw-cas -n $p -a -i "4 $g 3 0 0"

#   *  Tascell_membar
        ntimes $time ./spanning-lw-membar-statistics -n $p -a -i "4 $g 3 0 0"

#   *  Tascell_gcc_cas
#        ntimes $time ./spanning-gcc-cas -n $p -a -i "4 $g 3 0 0"

#   *  Tascell_gcc_membar
#        ntimes $time ./spanning-gcc-membar -n $p -a -i "4 $g 3 0 0"
    done
done


