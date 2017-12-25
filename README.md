# libunum - source code from Laslo Hunhold's Thesis

I have not verified that all the source code copied correctly from the publisheed pdf.  The code in the current state in libunum and problems compiles and runs.

Source code from the thesis published from 
<http://frign.de/publications/2016-11-08-the_unum_number_format.pdf>
and archived at 
<https://arxiv.org/abs/1701.00722v1>
and
<https://web.archive.org/web/20170107142232/http://frign.de/publications/2016-11-08-the_unum_number_format.pdf>

As referenced by
```
Hunhold, Laslo (2016-11-08). The Unum Number Format: Mathematical Foundations, Implementation and Comparison to IEEE 754 Floating-Point Numbers (PDF) (Bachelor thesis). Universität zu Köln, Mathematisches Institut. arXiv:1701.00722v1 Freely accessible. Archived (PDF) from the original on 2017-01-07. Retrieved 2016-10-23.
```


## Changes
        - added missing #include <fenv.h> to gen.c
        - removed utf chars copied from pdf
        - gen writes directly to named files unum.h and table.c
