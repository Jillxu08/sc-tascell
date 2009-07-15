#/bin/sh
graphs="1 2 3 4"
procs="1 2 3 4 5 6 7 8"
time=3

# $B!&I>2A(B + xy$B%0%i%U$J$I$r$U$/$a$F!$J?@P7/Cf?4$G$*4j$$$G$-$^$9$G$7$g$&$+(B?
#   1,2,3,4,5,6,7,8 $B%3%"$9$Y$F$GI>2A$9$l$P$h$$$+$H;W$$$^$9!%(B
#   $B$G$-$?$i$G$-$k$@$1J8>O$b(B? 

# $B!&AH$_9g$o$;$O(B
#    PARALLEL_SEARCH2 $B$O;H$o$J$$(B
#    PARALLEL_SEARCH $B$H(B PARALLEL_SEARCH3 $B$r;H$&!%(B

for g in $graphs; do
# #   *  serial ($BC`<!(BC$B$G!$C1$KD:E@%9%?%C%/$N$_(B search_s2)
#     ntimes $time ./st-serial $g

# # *  serial_call ($BC`<!(BC$B$G!$8F$S=P$7$HD:E@%9%?%C%/$r;H$&!$(B
# #          Tascell$B$d(BCilk$B$G$NJBNs2=$O$5$l$F$$$J$$!%(B
# #          cas_int, membar (XXX_to_start_read$B$J$I(B) $B$J$I$O;H$o$J$$!%(B)
#     ntimes $time ./st-serial-call $g 2

# #   *  serial_call_cas  ($BC`<!(BC$B$G!$8F$S=P$7$HD:E@%9%?%C%/$r;H$&!$(B
# #          Tascell$B$d(BCilk$B$G$NJBNs2=$O$5$l$F$$$J$$!%(B
# #          cas_int (PARALLEL_SEARCH )$B$r;H$&!%(B)
#     ntimes $time ./st-serial-call-cas $g 2

# #   *  serial_call_membar  ($BC`<!(BC$B$G!$8F$S=P$7$HD:E@%9%?%C%/$r;H$&!$(B
# #          Tascell$B$d(BCilk$B$G$NJBNs2=$O$5$l$F$$$J$$!%(B
# #          membar (PARALLEL_SEARCH3) $B$r;H$&!%(B)
#     ntimes $time ./st-serial-call-membar $g 2

    for p in $procs; do
#   *  Cilk_cas
        ntimes $time ./cilk/affinity $p ./cilk/st-par-cilk --nproc $p $g 2
#   *  Cilk_membar
        ntimes $time ./cilk/affinity $p ./cilk/st-par-cilk3 --nproc $p $g 2
        
# #   *  Tascell_cas    (list->array $B$O$I$&$7$^$7$g$&$+(B....)
#         ntimes $time ./spanning-lw-cas -n $p -a -i "4 $g 3 0 0"

# #   *  Tascell_membar
#         ntimes $time ./spanning-lw-membar -n $p -a -i "4 $g 3 0 0"

# #   *  Tascell_gcc_cas
#         ntimes $time ./spanning-gcc-cas -n $p -a -i "4 $g 3 0 0"

# #   *  Tascell_gcc_membar
#         ntimes $time ./spanning-gcc-membar -n $p -a -i "4 $g 3 0 0"

    done

#   *  SYNCHED $B$r;H$o$J$$(B Cilk $B$,$"$C$F$b$h$$$+$J(B...
done


# $B!&%0%i%U$K$"$H!$(Btree $B$rB-$7$F$b$i$($^$;$s$+(B?
#   $B@h=5$O>iD9$JLZ$H$$$&OC$r$7$^$7$?$,!$C1$J$kLZ$N$[$&$,$h$$$H;W$$(B
#   $BD>$7$^$7$?!%D:E@(B i $B$,(B $BD:E@(B (i-1)/2 $B$H;^$r:n$l$P$h$$$G$9!%(B
#   ($B2f!9$NJ}K!$G$O$H$/$KLdBj$J$$$G$9$,!$(B
#    $B4{B88&5f$N(B batching $B$,$&$^$/$$$+$J$$$N$O<+L@$J$N$G(B)

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

# -- 
# $BH,?y(B

