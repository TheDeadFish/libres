@call cmake_gcc.bat

:: install
@copy /Y bin\libres.exe %PROGRAMS%\bin
@copy /Y bin\libmerge.exe %PROGRAMS%\bin
@libmerge %PROGRAMS%\local\lib32\libexshit.a  bin\libres.a
@copy /Y arFile.h %PROGRAMS%\local\include 
@copy /Y object.h %PROGRAMS%\local\include 
