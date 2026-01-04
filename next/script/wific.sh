#!/bin/bash
# filepath: wifi_connect.sh

# WiFi connection script
# Usage: ./wifi_connect.sh <SSID> <PASSWORD>

# Check parameter count
if [ $# -ne 2 ]; then
    echo "Usage: $0 <SSID> <PASSWORD>"
    echo "Example: $0 \"MyWiFi\" \"mypassword\""
    echo "Return code:"
    echo "  0. Success"
    echo "  1. Invalid parameters. Please provide both SSID and password."
    echo "  2. No wireless interface found"
    echo "  3. Cannot find WiFi network with the specified SSID"
    echo "  4. Unable to obtain IP address"
    echo "  5. Incorrect WiFi password"
    echo "  6. Authentication timeout (signal may be weak or network busy)"
    echo "  7. Association failed (possible signal issue or network restriction)"
    echo "  8. Connection was disconnected"
    echo "  9. Unknown error"
    exit 1
fi

SSID="$1"
PASSWORD="$2"
#INTERFACE=$(iw dev | awk '$1=="Interface"{print $2}' | head -n1)
INTERFACE=wlan0

# Check if wireless interface is found
if [ -z "$INTERFACE" ]; then
    echo "Error: No wireless interface found"
    exit 2
fi

echo "Connecting to WiFi: $SSID"
echo "Using interface: $INTERFACE"

# Cleanup function
cleanup_and_exit() {
    rm -f "$TEMP_CONF"
    exit $1
}

# Check if network is visible
check_network_visibility() {
    echo "Scanning networks..."
    
    # First try to match SSID directly
    if iw dev "$INTERFACE" scan | grep -F "SSID: $SSID" > /dev/null 2>&1; then
        return 0
    fi
    
    # For non-ASCII SSID, convert to hex and search
    local ssid_hex=$(printf "%s" "$SSID" | hexdump -v -e '/1 "%02x"')
    
    # Scan and look for hex representation
    local scan_result=$(iw dev "$INTERFACE" scan 2>/dev/null)
    
    # Check both plain text and hex format
    if echo "$scan_result" | grep -qF "SSID: $SSID" || \
       echo "$scan_result" | grep -qi "$ssid_hex"; then
        return 0
    fi
    
    return 1
}

# Check if connected to specific SSID using iw
check_connection_status() {
    local target_ssid="$1"
    
    # Get current connection info
    local link_info=$(iw dev "$INTERFACE" link 2>/dev/null)
    
    # Check if interface is connected
    if ! echo "$link_info" | grep -q "Connected to"; then
        return 1
    fi
    
    # Method 1: Direct SSID match for ASCII SSIDs
    if echo "$link_info" | grep -qF "SSID: $target_ssid"; then
        return 0
    fi
    
    # Method 2: For non-ASCII SSIDs, convert to hex and compare
    local target_ssid_hex=$(printf "%s" "$target_ssid" | hexdump -v -e '/1 "%02x"')
    
    # Extract SSID line and check both formats
    local ssid_line=$(echo "$link_info" | grep "SSID:")
    
    # Check if hex representation matches
    if [ -n "$target_ssid_hex" ] && echo "$ssid_line" | grep -qi "$target_ssid_hex"; then
        return 0
    fi
    
    # Method 3: Alternative approach using iwconfig if available
    if command -v iwconfig >/dev/null 2>&1; then
        local iwconfig_info=$(iwconfig "$INTERFACE" 2>/dev/null | grep "ESSID:")
        if echo "$iwconfig_info" | grep -qF "\"$target_ssid\""; then
            return 0
        fi
    fi
    
    return 1
}

# Check if internet is accessible by pinging Google DNS and Baidu DNS
check_internet_connectivity() {
    echo "Testing internet connectivity..."
    if ping -c 1 -W 3 8.8.8.8 > /dev/null 2>&1; then
        echo "Internet connectivity test passed (Google DNS)"
        return 0
    elif ping -c 1 -W 3 180.76.76.76 > /dev/null 2>&1; then
        echo "Internet connectivity test passed (Baidu DNS)"
        return 0
    else
        echo "Internet connectivity test failed - no response from DNS servers"
        return 1
    fi
}

# Get signal strength using iw
get_signal_strength() {
    iw dev "$INTERFACE" link | grep "signal:" | awk '{print $2 " " $3}'
}

# Analyze wpa_supplicant log for detailed error information
analyze_connection_error() {
    local log_file="$1"
    
    if [ -f "$log_file" ]; then
        # Check for password error
        if grep -q "4-Way Handshake failed" "$log_file" || 
           grep -q "WRONG_KEY" "$log_file" || 
           grep -q "pre-shared key may be incorrect" "$log_file" || 
           grep -q "WPA: 4-Way Handshake failed" "$log_file"; then
            echo "Connection failed: Incorrect WiFi password"
            return 5
        fi
        
        # Check for network not found
        if grep -q "No network configuration found" "$log_file" || 
           grep -q "SSID.*not found" "$log_file"; then
            echo "Connection failed: Specified WiFi network not found"
            return 4
        fi
        
        # Check for authentication timeout
        if grep -q "authentication timed out" "$log_file" || 
           grep -q "Authentication timeout" "$log_file"; then
            echo "Connection failed: Authentication timeout (signal may be weak or network busy)"
            return 6
        fi
        
        # Check for association failure
        if grep -q "Association request to the driver failed" "$log_file" || 
           grep -q "association failed" "$log_file"; then
            echo "Connection failed: Association failed (possible signal issue or network restriction)"
            return 7
        fi
        
        # Check for other errors
        if grep -q "CTRL-EVENT-DISCONNECTED" "$log_file"; then
            echo "Connection failed: Connection was disconnected"
            return 8
        fi
    fi
    
    echo "Connection failed: Unknown error"
    return 9
}

killall -q dnsmasq 2>/dev/null || true
# Stop any existing hostapd processes
killall -q hostapd 2>/dev/null || true

# Disconnect current connection
wpa_cli -i "$INTERFACE" disconnect > /dev/null 2>&1

# Kill existing wpa_supplicant processes
killall -q wpa_supplicant > /dev/null 2>&1 || true
sleep 1

# First check if network is visible
if ! check_network_visibility; then
    echo "Connection failed: Cannot find WiFi network '$SSID'"
    echo "Please check:"
    echo "  1. SSID name is correct"
    echo "  2. Device is within WiFi signal range"
    echo "  3. WiFi is enabled"
    cleanup_and_exit 3
fi

echo "Found WiFi network: $SSID"

# Create temporary configuration file
TEMP_CONF=/data/wpa_supplicant.conf
WPA_LOG=/tmp/wpa_supplicant.log

cat > "$TEMP_CONF" << EOF
ctrl_interface=/var/run/wpa_supplicant
update_config=1

network={
    ssid="$SSID"
    psk="$PASSWORD"
    key_mgmt=WPA-PSK
    priority=1
}
EOF

# Bring up interface
ip link set "$INTERFACE" up

# Connect using wpa_supplicant with logging
echo "Attempting to connect..."
wpa_supplicant -B -i "$INTERFACE" -c "$TEMP_CONF" -f "$WPA_LOG" > /dev/null 2>&1

# Monitor connection status
TIMEOUT=15
COUNTER=0

while [ $COUNTER -lt $TIMEOUT ]; do
    # Check if connection is successful
    if check_connection_status "$SSID"; then
        echo "Connected to WiFi: $SSID"
        break
    fi
    
    # Check for errors
    if [ -f "$WPA_LOG" ]; then
        if grep -q "4-Way Handshake failed\|WRONG_KEY\|pre-shared key may be incorrect\|WPA: 4-Way Handshake failed" "$WPA_LOG"; then
            echo "Connection failed: Incorrect WiFi password"
            echo "Please check if the password is correct"
            cleanup_and_exit 5
        fi
        
        if grep -q "authentication timed out\|Authentication timeout" "$WPA_LOG"; then
            echo "Connection failed: Authentication timeout"
            echo "Possible causes: Weak signal, network busy, or router issues"
            cleanup_and_exit 7
        fi
    fi
    
    sleep 1
    COUNTER=$((COUNTER + 1))
    echo -n "."
done

echo ""

# Check final connection status
if ! check_connection_status "$SSID"; then
    analyze_connection_error "$WPA_LOG"
    ERROR_CODE=$?
    killall -q wpa_supplicant > /dev/null 2>&1 || true
    cleanup_and_exit $ERROR_CODE
fi

# Kill existing udhcpc processes
killall -q udhcpc > /dev/null 2>&1 || true

# Get IP address using udhcpc
echo "Obtaining IP address..."
udhcpc -i "$INTERFACE" -q -t 10 -T 3 > /dev/null 2>&1

# Wait for DHCP completion
sleep 3

# Check if IP address is obtained
IP_ADDRESS=$(ip addr show "$INTERFACE" | grep "inet " | awk '{print $2}' | cut -d'/' -f1)

if [ -n "$IP_ADDRESS" ]; then
    echo "Connection successful!"
    echo "Network name: $SSID"
    echo "IP address: $IP_ADDRESS"
    echo "Interface: $INTERFACE"
    
    # Get gateway address
    GATEWAY=$(ip route | grep "default.*$INTERFACE" | awk '{print $3}')
    if [ -n "$GATEWAY" ]; then
        echo "Gateway: $GATEWAY"
    fi
    
    # Get signal strength
    SIGNAL=$(get_signal_strength)
    if [ -n "$SIGNAL" ]; then
        echo "Signal strength: $SIGNAL"
    fi
    
    # Get connection details using iw
    FREQ=$(iw dev "$INTERFACE" link | grep "freq:" | awk '{print $2}')
    if [ -n "$FREQ" ]; then
        echo "Frequency: ${FREQ} MHz"
    fi

    # Test network connectivity
    if check_internet_connectivity; then
        echo "Network connectivity test passed"
    else
        echo "Network connectivity test failed, but IP address obtained"
    fi
    
    cleanup_and_exit 0
else
    echo "Connection failed: Unable to obtain IP address"
    echo "Possible causes:"
    echo "  1. DHCP server issues"
    echo "  2. Network configuration error" 
    echo "  3. IP address pool exhausted"
    killall -q wpa_supplicant > /dev/null 2>&1 || true
    cleanup_and_exit 4
fi
