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

Follow below steps to check volume and power button functionality in CIV-GVTD
=============================================================================
Step 1:
    1.1 Copy the vinput-manager and sendkey files into ~/civ/scripts folder
    1.2 $cd ~/civ/scripts
    1.3 $chmod +x vinput-manager sendkey

Step 2: sudo systemctl set-default multi-user.target

Step 3: Update file logind.conf with "HandlePowerKey=ignore" and reboot device
        sudo vim /etc/systemd/logind.conf <br>
        [Login] <br>
        HandlePowerKey=ignore <br>

Step 4: launch vinput-manager
        $sudo ./vinput-manager --gvtd <br>

Step 5:
     5.1. add below 3 lines under common_options variable in
          start_android_qcow2.sh script. <br>
       -device virtio-input-host-pci,evdev=/dev/input/by-id/Power-Button-vm0 \
       -device virtio-input-host-pci,evdev=/dev/input/by-id/Volume-Button-vm0 \
       -qmp unix:./qmp-vinput-sock,server,nowait \

     5.2. Launch CIV: $sudo ./scripts/start_android_qcow2.sh --gvtd
     5.3. Disbale 'Jump to Camera' setting in Android

Step 6:
    Run sendkey application to verify volume and power key functionality in android
    Volume Functionality:
            ./sendkey --vm 0 --volume up => Increases volume in CIV
            ./sendkey --vm 0 --volume down  => decreases volume in CIV
    Power Functionality:
            Disable "Jump to camera" option in android settings.
            ./sendkey --vm 0 --power 0  => Suspend/Resume in CIV
            ./sendkey –vm 0 –power 5    => long press of power key for 5 seconds.
                                           Displays power options in android
Note:
    1. Use sendkey application after launching CIV.
