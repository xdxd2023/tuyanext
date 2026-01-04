#!/bin/bash

# Function to display script usage
usage() {
    echo "Usage: $0 <ssid> [password]"
    echo "  <ssid>     : Name of the WiFi hotspot"
    echo "  [password] : Optional password (min 8 chars). If not provided, creates an open network"
    exit 1
}

# Check if hostapd is installed
check_requirements() {
    if ! command -v hostapd &> /dev/null; then
        echo "Error: hostapd is not installed. Please install it first."
        echo "sudo apt-get install hostapd dnsmasq"
        exit 1
    fi

    if ! command -v dnsmasq &> /dev/null; then
        echo "Error: dnsmasq is not installed. Please install it first."
        echo "sudo apt-get install hostapd dnsmasq"
        exit 1
    fi
}

# Get wireless interface
get_wireless_interface() {
    # Try to find wireless interface
    WIRELESS_INTERFACE=$(ip link | grep -Eo 'wlan[0-9]' | head -1)
    
    if [ -z "$WIRELESS_INTERFACE" ]; then
        echo "No wireless interface found."
        exit 1
    fi
    
    echo "Using wireless interface: $WIRELESS_INTERFACE"
    return 0
}

# Configure and start the hotspot
create_hotspot() {
    local SSID="$1"
    local PASSWORD="$2"
    local INTERFACE="$3"
    
    # Stop potential conflicting services
    killall -q wpa_supplicant 2>/dev/null || true
    
    # Create hostapd configuration file
    cat > /tmp/hostapd.conf << EOF
interface=$INTERFACE
driver=nl80211
ssid=$SSID
hw_mode=g
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
EOF

    # Add security settings if password is provided
    if [ -n "$PASSWORD" ]; then
        cat >> /tmp/hostapd.conf << EOF
wpa=2
wpa_passphrase=$PASSWORD
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
EOF
    fi
    
    # Create dnsmasq configuration
    cat > /tmp/dnsmasq.conf << EOF
interface=$INTERFACE
dhcp-range=192.168.176.2,192.168.176.20,255.255.255.0,24h
EOF
    
    # Enable IP forwarding
    # echo 1 > /proc/sys/net/ipv4/ip_forward
    
    killall -q dnsmasq 2>/dev/null || true
    # Stop any existing hostapd processes
    killall -q hostapd 2>/dev/null || true
    sleep 1
    # Flush existing IP addresses
    ip addr flush dev "$INTERFACE"
    # Set up the interface
    ip link set "$INTERFACE" up
    # Start DHCP server
    dnsmasq -C /tmp/dnsmasq.conf &
    # Start the access point
    hostapd -B /tmp/hostapd.conf
    sleep 1
    # Assign a static IP address
    ip addr add 192.168.176.1/24 dev "$INTERFACE"
    sleep 1
}

# Main script execution
check_requirements

# Check arguments
if [ $# -lt 1 ]; then
    usage
fi

SSID="$1"
PASSWORD="$2"

# Validate password if provided
if [ -n "$PASSWORD" ] && [ ${#PASSWORD} -lt 8 ]; then
    echo "Error: Password must be at least 8 characters long"
    exit 1
fi

# Get wireless interface
get_wireless_interface
INTERFACE=$WIRELESS_INTERFACE

echo "Setting up hotspot with SSID: $SSID"
if [ -z "$PASSWORD" ]; then
    echo "Creating an open network (no password)"
else
    echo "Hotspot will be secured with the provided password"
fi

# Create the hotspot
create_hotspot "$SSID" "$PASSWORD" "$INTERFACE"
