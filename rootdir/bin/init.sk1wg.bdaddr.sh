#!/vendor/bin/sh
#
# SPDX-FileCopyrightText: 2026 The JideRemixTabletRevived Project
# SPDX-License-Identifier: Apache-2.0
#
# The BCM43341 has no Bluetooth address programmed, and the Bluetooth HAL
# aborts without one ("Open: No Bluetooth Address!"). Stock Remix OS kept
# its address in /data. On the first boot make a random one (locally
# administered, unicast: first byte 0x02) and keep it from then on.

F=/data/vendor/bluetooth/bdaddr

if [ ! -s "$F" ]; then
    b=$(od -An -N5 -tx1 /dev/urandom | tr -d ' \n' | sed 's/\(..\)/:\1/g')
    echo "02$b" > "$F.tmp" && mv "$F.tmp" "$F"
fi

chown bluetooth:bluetooth "$F"
chmod 0444 "$F"
