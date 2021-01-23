multipass unmount ubuntu-vm
multipass mount %cd% ubuntu-vm:\temp
multipass exec ubuntu-vm -- bash -c "make -C temp %1" 
multipass unmount ubuntu-vm