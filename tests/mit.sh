#!/bin/bash

# Multi-threaded Server Integration Test
# Tests that the server can handle simultaneous requests

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
cd $SCRIPTPATH
echo "Script executed from: ${PWD}"
set -e

echo ""
echo "========== Building Server =========="
mkdir -p ../build
cd ../build && cmake .. && make
cd $SCRIPTPATH

SERVER_PID=0

shutdown_server() {
    # Make sure server actually started
    if [[ $SERVER_PID -ne 0 ]]; then
        echo "Stopping server..."
        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null || true
        sleep 0.3
    fi
}

trap shutdown_server EXIT

echo ""
echo "========== Creating Test Config =========="
# Create test config with SleepHandler
cat > ./test_multithreading_config << 'EOF'
server {
    listen 80;
    
    location /echo {
        handler EchoHandler;
    }
    
    location /sleep {
        handler SleepHandler;
        sleep_seconds 5;
    }
    
    location /static {
        handler StaticHandler;
        root /app/static;
    }
}
EOF
echo "Test config created"

echo ""
echo "========== Starting Server =========="
../build/bin/server ./test_multithreading_config &
SERVER_PID=$!
sleep 1  # give server time to start up

# --- Test 1: Verify SleepHandler blocks for expected time ---
echo ""
echo "========== Test 1: SleepHandler Blocks Correctly =========="

echo "Sending request to /sleep endpoint..."
START_TIME=$(date +%s)
curl -s -i localhost:80/sleep > ./tmp_sleep.txt
END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

echo "Request took $ELAPSED seconds"

# Check response is 200 OK
if ! grep -q "200 OK" ./tmp_sleep.txt; then
    echo "Test 1 failed - missing 200 OK status"
    echo "Actual output:"
    cat ./tmp_sleep.txt
    rm ./tmp_sleep.txt
    exit 1
fi

# Check it contains "Slept for" message
if ! grep -q "Slept for" ./tmp_sleep.txt; then
    echo "Test 1 failed - missing 'Slept for' message"
    echo "Actual output:"
    cat ./tmp_sleep.txt
    rm ./tmp_sleep.txt
    exit 1
fi

# Check it took approximately the right amount of time (allow ±1 second)
# Assuming sleep_seconds is configured as 5 in the config
if [[ $ELAPSED -lt 4 ]] || [[ $ELAPSED -gt 6 ]]; then
    echo "Test 1 failed - request took $ELAPSED seconds, expected ~5"
    rm ./tmp_sleep.txt
    exit 1
fi

rm ./tmp_sleep.txt
echo "Test 1 passed - SleepHandler blocks correctly"


# --- Test 2: Simultaneous Requests (Core Multi-threading Test) ---
echo ""
echo "========== Test 2: Server Handles Simultaneous Requests =========="

echo "Starting slow request to /sleep in background..."
START_TOTAL=$(date +%s.%N)

# Start slow request in background
curl -s -i localhost:80/sleep > ./tmp_slow.txt &
SLOW_PID=$!

# Give slow request time to start blocking
sleep 0.5

echo "Starting fast request to /echo (should NOT be blocked)..."
# Start fast request and measure its time
START_FAST=$(date +%s)
curl -s -i localhost:80/echo > ./tmp_fast.txt
END_FAST=$(date +%s)

# Calculate fast request time (integer seconds)
FAST_TIME=$((END_FAST - START_FAST))

echo "Fast request completed in $FAST_TIME seconds"

# Fast request should complete quickly (< 2 seconds)
FAST_TOO_SLOW=$((FAST_TIME >= 2))
if [[ $FAST_TOO_SLOW -eq 1 ]]; then
    echo "Test 2 failed - fast request took $FAST_TIME seconds (expected < 2)"
    echo "This indicates requests are being handled sequentially, not concurrently"
    wait $SLOW_PID 2>/dev/null || true
    rm ./tmp_slow.txt ./tmp_fast.txt
    exit 1
fi

# Verify fast request got proper response
if ! grep -q "200 OK" ./tmp_fast.txt; then
    echo "Test 2 failed - fast request didn't get 200 OK"
    echo "Actual output:"
    cat ./tmp_fast.txt
    wait $SLOW_PID 2>/dev/null || true
    rm ./tmp_slow.txt ./tmp_fast.txt
    exit 1
fi

# Wait for slow request to complete
wait $SLOW_PID

# Verify slow request got proper response
if ! grep -q "200 OK" ./tmp_slow.txt; then
    echo "Test 2 failed - slow request didn't get 200 OK"
    echo "Actual output:"
    cat ./tmp_slow.txt
    rm ./tmp_slow.txt ./tmp_fast.txt
    exit 1
fi

rm ./tmp_slow.txt ./tmp_fast.txt
echo "Test 2 passed - server handles simultaneous requests correctly"


# --- Test 3: Multiple Simultaneous Slow Requests ---
echo ""
echo "========== Test 3: Multiple Simultaneous Slow Requests =========="

echo "Starting 3 slow requests simultaneously..."
START_MULTI=$(date +%s)

# Launch 3 slow requests in parallel
curl -s -i localhost:80/sleep > ./tmp_multi1.txt &
PID1=$!
curl -s -i localhost:80/sleep > ./tmp_multi2.txt &
PID2=$!
curl -s -i localhost:80/sleep > ./tmp_multi3.txt &
PID3=$!

# Wait for all to complete
wait $PID1
wait $PID2
wait $PID3

END_MULTI=$(date +%s)
TOTAL_TIME=$((END_MULTI - START_MULTI))

echo "All 3 requests completed in $TOTAL_TIME seconds total"

# If sequential, would take 15+ seconds (3 * 5)
# If concurrent, should take ~5 seconds
if [[ $TOTAL_TIME -gt 10 ]]; then
    echo "Test 3 failed - took $TOTAL_TIME seconds, expected ~5"
    echo "This indicates requests are being handled sequentially"
    rm ./tmp_multi1.txt ./tmp_multi2.txt ./tmp_multi3.txt
    exit 1
fi

if [[ $TOTAL_TIME -lt 4 ]]; then
    echo "Test 3 failed - completed too quickly ($TOTAL_TIME seconds)"
    rm ./tmp_multi1.txt ./tmp_multi2.txt ./tmp_multi3.txt
    exit 1
fi

# Verify all got proper responses
for i in 1 2 3; do
    if ! grep -q "200 OK" ./tmp_multi${i}.txt; then
        echo "Test 3 failed - request $i didn't get 200 OK"
        echo "Actual output:"
        cat ./tmp_multi${i}.txt
        rm ./tmp_multi1.txt ./tmp_multi2.txt ./tmp_multi3.txt
        exit 1
    fi
done

rm ./tmp_multi1.txt ./tmp_multi2.txt ./tmp_multi3.txt
echo "Test 3 passed - multiple simultaneous requests handled concurrently"


# --- Test 4: Interleaved Fast and Slow Requests ---
echo ""
echo "========== Test 4: Interleaved Fast and Slow Requests =========="

echo "Starting interleaved pattern: slow, fast, slow, fast..."
START_INTERLEAVE=$(date +%s)

# Start slow request 1
curl -s -i localhost:80/sleep > ./tmp_interleave_slow1.txt &
SLOW1_PID=$!
sleep 0.2

# Start fast request 1 (should not be blocked by slow1)
START_FAST1=$(date +%s)
curl -s -i localhost:80/echo > ./tmp_interleave_fast1.txt
END_FAST1=$(date +%s)
FAST1_TIME=$((END_FAST1 - START_FAST1))

# Start slow request 2
curl -s -i localhost:80/sleep > ./tmp_interleave_slow2.txt &
SLOW2_PID=$!
sleep 0.2

# Start fast request 2 (should not be blocked by slow1 or slow2)
START_FAST2=$(date +%s)
curl -s -i localhost:80/echo > ./tmp_interleave_fast2.txt
END_FAST2=$(date +%s)
FAST2_TIME=$((END_FAST2 - START_FAST2))

# Wait for slow requests
wait $SLOW1_PID
wait $SLOW2_PID

END_INTERLEAVE=$(date +%s)

echo "Fast request 1 took $FAST1_TIME seconds"
echo "Fast request 2 took $FAST2_TIME seconds"

# Both fast requests should be quick (< 2 seconds each)
FAST1_TOO_SLOW=$((FAST1_TIME >= 2))
FAST2_TOO_SLOW=$((FAST2_TIME >= 2))

if [[ $FAST1_TOO_SLOW -eq 1 ]] || [[ $FAST2_TOO_SLOW -eq 1 ]]; then
    echo "Test 4 failed - fast requests were blocked"
    echo "Fast1: $FAST1_TIME sec, Fast2: $FAST2_TIME sec"
    rm ./tmp_interleave_*.txt
    exit 1
fi

# Verify all responses
for file in ./tmp_interleave_*.txt; do
    if ! grep -q "200 OK" "$file"; then
        echo "Test 4 failed - $file didn't get 200 OK"
        cat "$file"
        rm ./tmp_interleave_*.txt
        exit 1
    fi
done

rm ./tmp_interleave_*.txt
echo "Test 4 passed - interleaved requests handled correctly"


echo ""
echo "=========================================="
echo "All multi-threading tests passed!"
echo "Server successfully handles concurrent requests"
echo "=========================================="

exit 0
