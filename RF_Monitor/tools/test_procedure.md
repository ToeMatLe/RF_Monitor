Types of Captures:

python3 tools/capture_rssi.py \
  --port /dev/cu.usbmodem1303 \
  --label background \
  --windows 100

Background 100 times (Location 1)
Background 100 times (Location 2)

python3 tools/capture_rssi.py \
  --port /dev/cu.usbmodem1303 \
  --label continious \
  --windows 100

Continious 100 times (Distance 0m)
Continious 100 times (Distance 5m)
Continious 100 times (Distance 10m)
Continious 100 times (Distance 20m)

python3 tools/capture_rssi.py \
  --port /dev/cu.usbmodem1303 \
  --label short_burst \
  --windows 100

Short Burst 100 times (Distance 0m)
Short Burst 100 times (Distance 5m)
Short Burst 100 times (Distance 10m)
Short Burst 100 times (Distance 20m)

python3 tools/capture_rssi.py \
  --port /dev/cu.usbmodem1303 \
  --label periodic_burst \
  --windows 100

Periodic Burst 100 times (Distance 0m)
Periodic Burst 100 times (Distance 5m)
Periodic Burst 100 times (Distance 10m)
Periodic Burst 100 times (Distance 20m)