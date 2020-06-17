How to Compile
==============
Prerequisites:
    Minimum gcc version required is 7.4.0
    Ubuntu Host version is 18.04
Compile:
    $make
    Compilation generates vinput-manager & sendkey executables.

Follow below steps to check volume and power button functionality in CIV
========================================================================
Step 1:
        1.1 Copy the vinput-manager and sendkey files into ~/civ folder
        1.2 $cd ~/civ/
        1.3 $chmod +x vinput-manager sendkey

Step 2: launch vinput-manager
    $sudo ./vinput-manager

Step 3:
    3.1. add below two lines under common_options variable in start_android_qcow2.sh script.
	-device virtio-input-host-pci,evdev=/dev/input/by-id/Power-Button-vm0 \
	-device virtio-input-host-pci,evdev=/dev/input/by-id/Volume-Button-vm0 \
    3.2. Launch CIV. $sudo ./scripts/start_android_qcow2.sh
    3.3. Disbale 'Jump to Camera' setting in Android

Step 4:
    Run sendkey application to verify volume and power key functionality in android
    Volume Functionality:
            ./sendkey --vm 0 --volume up => Increases volume in CIV
            ./sendkey --vm 0 --volume down  => decreases volume in CIV
    Power Functionality:
            Disable "Jump to camera" option in android settings.
            ./sendkey --vm 0 --power 0	=> Suspend/Resume in CIV
		    ./sendkey –vm 0 –power 5	=> long press of power key for 5 seconds.
                                           Displays power options in android
Note:
    1. Use sendkey application after launching CIV.

Known Issues:
    Stale events for volume button.
