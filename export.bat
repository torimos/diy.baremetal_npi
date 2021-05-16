ssh rock64@rock64.local rm -rf /home/rock64/temp; mkdir /home/rock64/temp
scp -r %cd%/ rock64@rock64.local:/home/rock64/temp
@rem scp -r %cd% rock64@rock64.local:/home/rock64/projects

@REM cd %cd%/tools/bin/
@REM loader.exe -v -t64 -snsih_200.bin -o %cd%/../../build/nanopim3/kernel8_nsih.img %cd%/../../build/nanopim3/kernel8.img