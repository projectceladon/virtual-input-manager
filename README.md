Virtual Input manger uses uinput framework to create virtual/user-space input devices & virtio-input infrastructure from QEmu to map the devices on virtual machines

# How to Compile

Prerequisite:<br>
  *  Required minimum gcc version is 7.4.0 or Ubuntu 18.04 release of Ubuntu Host<br>

Compile:  <br>
  *  $make clean<br>
  *  $make<br>
  *  Compilation generates vinput-manager & sendkey executables.<br><br>

# Follow below steps to check volume and power button functionality in CIV-GVTG

Step 1:
  *  Pre-built binaries are placed in ~/civ/scripts directory  <br>
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
  2. To set device in to graphical mode.<br>
        $sudo systemctl set-default graphical.target <br>
        $sudo reboot <br>

# Follow below steps to check volume and power button functionality in CIV-GVTD

Step 1: Pre-built binaries are placed in ~/civ/scripts directory  <br>
  *  $cd ~/civ/scripts <br>
  *  $chmod +x vinput-manager sendkey <br>

Step 2: Set the device in multi-user target mode. <br> 
  *  $sudo systemctl set-default multi-user.target <br>

Step 3: Update logind.conf file with "HandlePowerKey=ignore" and reboot device <br>
  *  $sudo vim /etc/systemd/logind.conf <br>
        [Login] <br>
        HandlePowerKey=ignore <br>
  *  $sudo reboot

Step 4: Launch vinput-manager <br>
  *  $sudo ./vinput-manager --gvtd <br>

Step 5:<br>
  *  Add below 3 lines under common_options variable in start_android_qcow2.sh script. <br>
           -device virtio-input-host-pci,evdev=/dev/input/by-id/Power-Button-vm0 \    <br>
           -device virtio-input-host-pci,evdev=/dev/input/by-id/Volume-Button-vm0 \   <br>
           -qmp unix:./qmp-vinput-sock,server,nowait \ <br>

  *  Launch CIV: $sudo ./scripts/start_android_qcow2.sh --gvtd <br>

Step 6: Run sendkey application to verify volume and power key functionality in android <br>
  *  Volume Functionality:<br>
            ./sendkey --vm 0 --volume up => Increases volume in CIV<br>
            ./sendkey --vm 0 --volume down  => decreases volume in CIV<br>
  *  Power Functionality:<br>
            ./sendkey --vm 0 --power 0  => Suspend/Resume in CIV<br>
            ./sendkey –vm 0 –power 5    => long press of power key for 5 seconds. Displays power options in android. <br>

Note:<br>
    1. Use sendkey application after launching CIV.<br>
