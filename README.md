Virtual Input manger uses uinput framework to create virtual/user-space input devices & virtio-input infrastructure from QEmu to map the devices on virtual machines

# How to Compile

Prerequisite:<br>
  *  Required minimum gcc version is 7.4.0 or Ubuntu 18.04 release of Ubuntu Host<br>

Compile:  <br>
  *  $make clean<br>
  *  $make<br>
  *  Compilation generates vinput-manager & sendkey executables.<br><br>

# Follow below steps to check volume and power button functionality in CIV

Step 1:
  *  Copy the vinput-manager and sendkey files into ~/civ/scripts directory  <br>
  *  $cd ~/civ/scripts/  <br>
  *  $chmod +x vinput-manager sendkey  <br>

Step 2: Launch vinput-manager
  *  $sudo ./vinput-manager

Step 3: Launch Android
  * Add below two lines under common_options variable in start_android_qcow2.sh script.<br>
           -device virtio-input-host-pci,evdev=/dev/input/by-id/Power-Button-vm0 \ <br>
           -device virtio-input-host-pci,evdev=/dev/input/by-id/Volume-Button-vm0 \ <br>
  * Launch CIV using start_android_qcow2.sh script.

Step 4:
    Use sendkey application to verify volume and power key functionality in android<br>
  *  Volume Functionality:<br>
        ./sendkey --vm 0 --volume up => Increases volume in CIV<br>
        ./sendkey --vm 0 --volume down  => decreases volume in CIV<br>
  *  Power Functionality:<br>
        ./sendkey --vm 0 --power 0  => Short press to trigger Suspend/Resume in CIV<br>
        ./sendkey –vm 0 –power 5  => long press of power key for 5 seconds. Displays power options in Android.<br>

Note:<br>
  1. Use sendkey application only after launching CIV.<br>
