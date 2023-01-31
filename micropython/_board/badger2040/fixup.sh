SRC_DIR=$1
DST_DIR=$2

echo "Applying rp2_pio_c.patch"
cd "$DST_DIR"
git apply "$SRC_DIR/rp2_pio_c.patch"

echo "Applying wakeup_gpio.patch"
cd "$DST_DIR/../../lib/pico-sdk"
git apply "$SRC_DIR/wakeup_gpio.patch"
