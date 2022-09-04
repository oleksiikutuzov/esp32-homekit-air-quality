Import("env")
from shutil import copyfile

def move_bin(*args, **kwargs):
    print("Copying bin output to project directory...")
    target = str(kwargs['target'][0])
    print(target)
    if target == ".pio/build/esp32dev_v3/firmware.bin":
        copyfile(target, 'esp32_air_quality_v3.bin')
    elif target == ".pio/build/esp32dev_v4/firmware.bin":
        copyfile(target, 'esp32_air_quality_v4.bin')
    print("Done.")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", move_bin)   #post action for .bin