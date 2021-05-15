# nsdos - North Star DOS Disk Utility

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/3953578fd1654c26922d7115565afc6c)](https://www.codacy.com/gh/hharte/nsdos/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=hharte/nsdos&amp;utm_campaign=Badge_Grade)

North Star Computers had their own DOS for use with their Horizon and Advantage computer systems, as well as for use with their disk controller.  These disks have a very simple file system, where every file has to be contiguous.

`nsdos` can list and Extract Files from North Star disk images in Dave Dunfield’s [.nsi format](http://dunfield.classiccmp.org/img/). The .nsi format is commonly used in the [SIMH/AltairZ80](https://schorn.ch/altair.html) simulator, as well as with [FluxEngine for North Star](https://github.com/hharte/fluxengine/tree/northstar).

The filename in North Star DOS can be up to eight characters.  Each file has an associated file type, from 0-127, although only four are currently defined:


<table>
  <tr>
   <td>Type
   </td>
   <td>Description
   </td>
   <td>Metadata
   </td>
   <td>Output Filename
   </td>
  </tr>
  <tr>
   <td>0
   </td>
   <td>Default
   </td>
   <td>unused
   </td>
   <td>fname.DEFAULT
   </td>
  </tr>
  <tr>
   <td>1
   </td>
   <td>Binary Object Code
   </td>
   <td>Load Address
   </td>
   <td>fname.OBJECT_L&lt;load_address>
   </td>
  </tr>
  <tr>
   <td>2
   </td>
   <td>BASIC source file
   </td>
   <td>Actual File Size
   </td>
   <td>fname.BASIC
   </td>
  </tr>
  <tr>
   <td>3
   </td>
   <td>BASIC data file
   </td>
   <td>unused
   </td>
   <td>fname.BASIC_DATA
   </td>
  </tr>
</table>


In addition to the file name and type, the North Star DOS filesystem stores metadata about certain types of files.  For executable object code, the “load address” is stored in the metadata.  To preserve this important information, `nsdos` appends the load address to the output filename extension.


# Usage


```
North Star DOS File Utility (c) 2021 - Howard M. Harte

usage is: nsdos <filename.nsi> [command] [<filename>|<path>]
        <filename.nsi> North Star DOS Disk Image in .nsi format.
        [command]      LI - List files
                       EX - Extract files to <path>
If no command is given, LIst is assumed.
```



## To list files on a North Star DOS disk image:


```
$ nsdos <filename.nsi>
```


Or


```
$ nsdos <filename.nsi> li
```


For example:


```
$ nsdos North_Star_DOS_BASIC_2_2_1_DQ_System_Disk_PN03066A.nsi
North Star DOS File Utility (c) 2021 - Howard M. Harte

Filename  DA BLKS D TYP Type          Metadata
DOSBASIC   0    8 D   0 Default       08,00,51
SYSTEM     0    0 D   0 Default       00,00,00
211DQ      0    0 D   0 Default       00,00,00
03066A     0    0 D   0 Default       00,00,00
03067A     0    0 D   0 Default       00,00,00
<*>        0    8 D   3 BASIC Data    08,00,00
DOS        4   14 D   0 Default       07,20,20
BASIC     11   56 D   1 Object Code   Load addr: 1000
CD        39    4 D   1 Object Code   Load addr: 1000
CF        41    6 D   1 Object Code   Load addr: 1000
CK        44    4 D   1 Object Code   Load addr: 1000
CO        46    8 D   1 Object Code   Load addr: 1000
DT        50    4 D   1 Object Code   Load addr: 1000
M1000     52    8 D   1 Object Code   Load addr: 1000
RAMTEST3  56    4 D   1 Object Code   Load addr: 3000
RAMTEST5  58    4 D   1 Object Code   Load addr: 5000
MOVER     60   18 D   2 BASIC Program Actual Size: 18
SYSGEN    69   58 D   2 BASIC Program Actual Size: 58
-DOS      98   14 D   3 BASIC Data    07,20,20
-CD      105    4 D   3 BASIC Data    02,20,20
-CF      107    6 D   3 BASIC Data    03,20,20
-CK      110    4 D   3 BASIC Data    02,20,20
-CO      112    8 D   3 BASIC Data    04,20,20
-DT      116    4 D   3 BASIC Data    02,20,20
-MONITOR 118    8 D   3 BASIC Data    04,20,20
-BASIC   122   56 D   3 BASIC Data    38,00,00
FPBASIC  150   54 D   1 Object Code   Load addr: 1000
BASIC10  177   56 D   1 Object Code   Load addr: 1000
FPBAS10  205   54 D   1 Object Code   Load addr: 1000
BASIC12  232   58 D   1 Object Code   Load addr: 1000
FPBAS12  261   54 D   1 Object Code   Load addr: 1000
BASIC14  288   58 D   1 Object Code   Load addr: 1000
FPBAS14  317   54 D   1 Object Code   Load addr: 1000
-FPBASIC 344   54 D   3 BASIC Data    36,00,00
-BASIC10 371   56 D   3 BASIC Data    38,00,00
-FPBAS10 399   54 D   3 BASIC Data    36,00,00
-BASIC12 426   58 D   3 BASIC Data    3A,00,00
-FPBAS12 455   54 D   3 BASIC Data    36,00,00
-BASIC14 482   58 D   3 BASIC Data    3A,00,00
-FPBAS14 511   54 D   3 BASIC Data    36,00,00
EQUS     538   52 D   0 Default       34,00,00
```



## To Extract All Files from the disk image:


```
$ nsdos <filename.nsi> ex <path>
```


For example:


```
$ ./nsdos North_Star_DOS_BASIC_2_2_1_DQ_System_Disk_PN03066A.nsi ex out
North Star DOS File Utility (c) 2021 - Howard M. Harte

     DOS -> out/DOS.DEFAULT (3584 bytes)
   BASIC -> out/BASIC.OBJECT_L1000 (14336 bytes)
      CD -> out/CD.OBJECT_L1000 (1024 bytes)
      CF -> out/CF.OBJECT_L1000 (1536 bytes)
      CK -> out/CK.OBJECT_L1000 (1024 bytes)
      CO -> out/CO.OBJECT_L1000 (2048 bytes)
      DT -> out/DT.OBJECT_L1000 (1024 bytes)
   M1000 -> out/M1000.OBJECT_L1000 (2048 bytes)
RAMTEST3 -> out/RAMTEST3.OBJECT_L3000 (1024 bytes)
RAMTEST5 -> out/RAMTEST5.OBJECT_L5000 (1024 bytes)
   MOVER -> out/MOVER.BASIC (4608 bytes)
  SYSGEN -> out/SYSGEN.BASIC (14848 bytes)
    -DOS -> out/-DOS.BASIC_DATA (3584 bytes)
     -CD -> out/-CD.BASIC_DATA (1024 bytes)
     -CF -> out/-CF.BASIC_DATA (1536 bytes)
     -CK -> out/-CK.BASIC_DATA (1024 bytes)
     -CO -> out/-CO.BASIC_DATA (2048 bytes)
     -DT -> out/-DT.BASIC_DATA (1024 bytes)
-MONITOR -> out/-MONITOR.BASIC_DATA (2048 bytes)
  -BASIC -> out/-BASIC.BASIC_DATA (14336 bytes)
 FPBASIC -> out/FPBASIC.OBJECT_L1000 (13824 bytes)
 BASIC10 -> out/BASIC10.OBJECT_L1000 (14336 bytes)
 FPBAS10 -> out/FPBAS10.OBJECT_L1000 (13824 bytes)
 BASIC12 -> out/BASIC12.OBJECT_L1000 (14848 bytes)
 FPBAS12 -> out/FPBAS12.OBJECT_L1000 (13824 bytes)
 BASIC14 -> out/BASIC14.OBJECT_L1000 (14848 bytes)
 FPBAS14 -> out/FPBAS14.OBJECT_L1000 (13824 bytes)
-FPBASIC -> out/-FPBASIC.BASIC_DATA (13824 bytes)
-BASIC10 -> out/-BASIC10.BASIC_DATA (14336 bytes)
-FPBAS10 -> out/-FPBAS10.BASIC_DATA (13824 bytes)
-BASIC12 -> out/-BASIC12.BASIC_DATA (14848 bytes)
-FPBAS12 -> out/-FPBAS12.BASIC_DATA (13824 bytes)
-BASIC14 -> out/-BASIC14.BASIC_DATA (14848 bytes)
-FPBAS14 -> out/-FPBAS14.BASIC_DATA (13824 bytes)
    EQUS -> out/EQUS.DEFAULT (13312 bytes)
Extracted 35 files.
