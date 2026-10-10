#!/bin/sh
# hostap.sh - Tuya provisioning hotspot on wlan0.
#
# This board only has one usable WiFi netdev (wlan0); the WS73 driver rejects
# dynamic extra AP interfaces and the ini VAP_COUNT switch is not supported by
# this driver build, so the provisioning AP takes over wlan0.  The station
# configuration (netplan) is backed up and restored by "hostap.sh stop" so the
# device can reconnect to the router afterwards.
#
# Usage: ./hostap.sh <ssid> [password] [channel]
#        ./hostap.sh stop
#        ./hostap.sh status

IFACE=wlan0
NETPLAN_CONF=/etc/netplan/52-wlan0-init.yaml
HOSTAP_BACKUP=/etc/netplan/52-wlan0-init.yaml.hostap.bak

SSID=""
PASSWORD=""
CHAN=""

usage() {
    echo "Usage: $0 <ssid> [password] [channel] | stop | status"
    exit 1
}

stop_hotspot() {
    echo "Stopping hotspot..."
    killall -q hostapd 2>/dev/null || true
    killall -q dnsmasq 2>/dev/null || true
    ip addr flush dev "$IFACE" 2>/dev/null || true
    ip link set "$IFACE" down 2>/dev/null || true

    # restore the saved station configuration
    if [ -f "$HOSTAP_BACKUP" ]; then
        mv -f "$HOSTAP_BACKUP" "$NETPLAN_CONF"
    fi
    systemctl start systemd-networkd 2>/dev/null || true
    netplan apply 2>/dev/null || systemctl restart systemd-networkd 2>/dev/null || true

    # wait until wpa_supplicant is usable again (wific.sh needs wpa_cli)
    i=0
    while [ $i -lt 25 ]; do
        wpa_cli -i "$IFACE" ping >/dev/null 2>&1 && break
        i=$((i + 1))
        sleep 1
    done
    echo "Hotspot stopped, station config restored."
    return 0
}

status_hotspot() {
    if pgrep -x hostapd >/dev/null 2>&1; then
        echo "hostapd: RUNNING"
        [ -f /tmp/hostapd.conf ] && echo "SSID: $(grep '^ssid=' /tmp/hostapd.conf | cut -d= -f2)"
    else
        echo "hostapd: NOT RUNNING"
    fi
    if pgrep -x dnsmasq >/dev/null 2>&1; then
        echo "dnsmasq: RUNNING"
    else
        echo "dnsmasq: NOT RUNNING"
    fi
    ip addr show "$IFACE" 2>/dev/null | grep -E 'inet |state '
}

ACTION="$1"
[ "$ACTION" = "stop" ] && { stop_hotspot; exit 0; }
[ "$ACTION" = "status" ] && { status_hotspot; exit 0; }
[ -z "$ACTION" ] && usage

SSID="$1"
PASSWORD="$2"
CHAN="$3"
[ -z "$CHAN" ] && CHAN=6

if [ -n "$PASSWORD" ] && [ ${#PASSWORD} -lt 8 ]; then
    echo "Error: Password must be at least 8 characters long"
    exit 1
fi

if [ ! -d /sys/class/net/$IFACE ]; then
    echo "Error: $IFACE interface not found."
    exit 1
fi

echo "Setting up hotspot on $IFACE (channel $CHAN) with SSID: $SSID"

# Back up the station netplan config before releasing wlan0
if [ -f "$NETPLAN_CONF" ]; then
    cp -f "$NETPLAN_CONF" "$HOSTAP_BACKUP"
    rm -f "$NETPLAN_CONF"
fi
rm -f /run/systemd/network/10-netplan-$IFACE.network 2>/dev/null || true

# Release wlan0 from networkd / wpa_supplicant
systemctl stop systemd-networkd 2>/dev/null || true
killall -q wpa_supplicant 2>/dev/null || true
killall -q hostapd 2>/dev/null || true
killall -q dnsmasq 2>/dev/null || true
sleep 1

cat > /tmp/hostapd.conf << EOF
interface=$IFACE
driver=nl80211
ssid=$SSID
hw_mode=g
channel=$CHAN
wmm_enabled=0
macaddr_acl=0
auth_algs=1
EOF

if [ -n "$PASSWORD" ]; then
    cat >> /tmp/hostapd.conf << EOF
wpa=2
wpa_passphrase=$PASSWORD
wpa_key_mgmt=WPA-PSK
wpa_pairwise=CCMP
rsn_pairwise=CCMP
EOF
fi

cat > /tmp/dnsmasq.conf << EOF
port=0
interface=$IFACE
bind-interfaces
except-interface=lo
dhcp-range=192.168.176.2,192.168.176.20,255.255.255.0,24h
EOF

ip link set "$IFACE" down 2>/dev/null || true
ip addr flush dev "$IFACE" 2>/dev/null || true

hostapd -B /tmp/hostapd.conf
sleep 1

ip link set "$IFACE" up
ip addr add 192.168.176.1/24 dev "$IFACE" 2>/dev/null || true
sleep 1

mkdir -p /var/lib/misc
touch /var/lib/misc/dnsmasq.leases
dnsmasq -C /tmp/dnsmasq.conf &
sleep 1

if pgrep -x hostapd >/dev/null 2>&1; then
    echo "Hotspot started: SSID=$SSID iface=$IFACE gw=192.168.176.1"
    exit 0
else
    echo "Error: hostapd failed to start"
    exit 1
fi
