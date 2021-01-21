multipass unmount ubuntu-vm
multipass mount %cd% ubuntu-vm:\temp
multipass exec ubuntu-vm -- bash -c "BOARD=%1 make -C temp" 
multipass unmount ubuntu-vm