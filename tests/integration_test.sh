#!/bin/bash

#if mkdir: cannot create directory ‘/app’: Permission denied
# sudo mkdir /app
# sudo chown $(whoami):$(id -gn) /app
#

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
cd $SCRIPTPATH
echo "Script executed from: ${PWD}"
set -e
mkdir -p ../build
# cd ../build && cmake &>/dev/null .. && make &>/dev/null
cd ../build && cmake .. && make
cd $SCRIPTPATH


# --- Setup: Create temporary static directory and file ---
echo ""
echo "========== Setup: Creating test environment =========="
STATIC_DIR="/app/static"
mkdir -p "$STATIC_DIR"
echo "Hello, World!" > "$STATIC_DIR/index.html"
echo "Created $STATIC_DIR/index.html with test content"

CRUD_DIR="/app/crud_data"
mkdir -p "$CRUD_DIR"
echo "Created $CRUD_DIR for CRUD handler storage"

SERVER_PID=0

# Function to shutdown the server
shutdown_server() {
    # Make sure server actually started
    if [[ $SERVER_PID -ne 0 ]]; then
        echo "Stopping server..."
        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null || true
        sleep 0.3
    fi
}

# Ensure cleanup runs on EXIT or if the script is interrupted
trap shutdown_server EXIT

echo "Starting server..."
../build/bin/server ../configs/server_config &
SERVER_PID=$!
sleep 1  # give server time to start up


# --- Test 1: Good request ---
echo ""
echo "========== Test 1: Good Request =========="

echo "Sending echo request..."
curl -s -i localhost:80/echo > ./tmp.txt
CURL_EXIT=$?
# Check curl exit code
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT"
    exit 1
fi

echo "Comparing results..."
if cmp -s ./expected.txt ./tmp.txt; then
    rm ./tmp.txt
    echo "Test 1 passed."
else
    echo "Test 1 failed - content not same"
    echo "Actual output:"
    cat ./tmp.txt
    exit 1
fi


# --- Test 2: Bad request ---
echo ""
echo "========== Test 2: Bad Request =========="

echo "Sending malformed request..."
cat ./bad_request.txt | nc -C localhost 80 -q 0 > ./tmp2.txt

echo "Comparing results..."
if grep -q "400 Bad Request" ./tmp2.txt; then
    rm ./tmp2.txt
    echo "Test 2 passed."
else
    echo "Test 2 failed - content not same"
    echo "Actual output:"
    cat ./tmp2.txt
    exit 1
fi


# --- Test 3: GET file request ---
echo ""
echo "========== Test 3: GET File Request =========="

echo "Sending GET request for static/index.html..."
curl -s -i localhost:80/static/index.html > ./tmp3.txt
CURL_EXIT=$?
# Check curl exit code
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT"
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null || true
    exit 1
fi

echo "Checking response..."
# Check if response contains "Hello, World!"
if grep -q "Hello, World!" ./tmp3.txt; then
    # Also check for 200 OK status
    if grep -q "200 OK" ./tmp3.txt; then
        rm ./tmp3.txt
        echo "Test 3 passed."
    else
        echo "Test 3 failed - missing 200 OK status"
        echo "Actual output:"
        cat ./tmp3.txt
        exit 1
    fi
else
    echo "Test 3 failed - content does not contain 'Hello, World!'"
    echo "Actual output:"
    cat ./tmp3.txt
    exit 1
fi


# --- Test 4: GET non-existing file (404) ---
echo ""
echo "========== Test 4: GET Non-Existing File =========="

echo "Sending GET request for non-existing file..."
curl -s -i localhost:80/static/nonexistent.html > ./tmp4.txt
CURL_EXIT=$?
# Check curl exit code
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT"
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null || true
    exit 1
fi

echo "Checking response..."
# Check for 404 Not Found status
if grep -q "404 Not Found" ./tmp4.txt; then
    rm ./tmp4.txt
    echo "Test 4 passed."
else
    echo "Test 4 failed - missing 404 status"
    echo "Actual output:"
    cat ./tmp4.txt
    exit 1
fi

# --- Test 5: GET non-existing path (404) ---
echo ""
echo "========== Test 5: GET Non-Existing Path =========="

echo "Sending GET request for non-existing file..."
curl -s -i localhost:80/nonexistent/path.html > ./tmp5.txt
CURL_EXIT=$?
# Check curl exit code
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT"
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null || true
    exit 1
fi

echo "Checking response..."
# Check for 404 Not Found status
if grep -q "404 Not Found" ./tmp5.txt; then
    rm ./tmp5.txt
    echo "Test 5 passed."
else
    echo "Test 5 failed - missing 404 status"
    echo "Actual output:"
    cat ./tmp5.txt
    exit 1
fi

# --- Test 6: CRUD handler integration (POST, GET, DELETE) ---
echo ""
echo "========== Test 6: CRUD Handler POST / GET / DELETE =========="

# 6a. Create an entity via POST /api/Products
echo "Creating entity via POST /api/Products..."
curl -s -i -X POST "localhost:80/api/Products" \
     -H "Content-Type: application/json" \
     -d '{"name":"IntegrationTestProduct","price":8.40}' > ./tmp6_post.txt
CURL_EXIT=$?
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT during POST"
    exit 1
fi

# Check for 201 Created and JSON content type
if ! grep -q "201 Created" ./tmp6_post.txt; then
    echo "Test 6 failed - POST did not return 201 Created"
    echo "Actual output:"
    cat ./tmp6_post.txt
    exit 1
fi
if ! grep -qi "Content-Type: application/json" ./tmp6_post.txt; then
    echo "Test 6 failed - POST missing application/json Content-Type"
    echo "Actual output:"
    cat ./tmp6_post.txt
    exit 1
fi

# Extract ID from response body (assumes body is last line: {"id": <number>})
POST_BODY_LINE=$(tail -n 1 ./tmp6_post.txt)
ENTITY_ID=$(echo "$POST_BODY_LINE" | sed -E 's/.*"id":[[:space:]]*([0-9]+).*/\1/')
if [[ -z "$ENTITY_ID" ]]; then
    echo "Test 6 failed - could not parse entity ID from POST response"
    echo "Body line: $POST_BODY_LINE"
    exit 1
fi
echo "Created entity with ID: $ENTITY_ID"

# 6b. Retrieve the entity via GET /api/Products/{id}
echo "Retrieving entity via GET /api/Products/$ENTITY_ID..."
curl -s -i "localhost:80/api/Products/${ENTITY_ID}" > ./tmp6_get.txt
CURL_EXIT=$?
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT during GET"
    exit 1
fi

# Check 200 OK and JSON content type and body contents
if ! grep -q "200 OK" ./tmp6_get.txt; then
    echo "Test 6 failed - GET did not return 200 OK"
    echo "Actual output:"
    cat ./tmp6_get.txt
    exit 1
fi
if ! grep -qi "Content-Type: application/json" ./tmp6_get.txt; then
    echo "Test 6 failed - GET missing application/json Content-Type"
    echo "Actual output:"
    cat ./tmp6_get.txt
    exit 1
fi
if ! grep -q "IntegrationTestProduct" ./tmp6_get.txt; then
    echo "Test 6 failed - GET body does not contain expected entity data"
    echo "Actual output:"
    cat ./tmp6_get.txt
    exit 1
fi

# 6c. Update the entity w PUT /api/Products/{id}
UPDATED_JSON='{"name":"UpdatedIntegrationProduct","price":9.99}'
echo "Updating entity via PUT /api/Products/$ENTITY_ID..."
curl -s -i -X PUT "localhost:80/api/Products/${ENTITY_ID}" \
     -H "Content-Type: application/json" \
     -d "$UPDATED_JSON" > ./tmp6_put.txt
CURL_EXIT=$?
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT during PUT"
    exit 1
fi

if ! grep -q "200 OK" ./tmp6_put.txt; then
    echo "Test 6 failed - PUT did not return 200 OK"
    echo "Actual output:"
    cat ./tmp6_put.txt
    exit 1
fi
if ! grep -qi "Content-Type: application/json" ./tmp6_put.txt; then
    echo "Test 6 failed - PUT missing application/json Content-Type"
    echo "Actual output:"
    cat ./tmp6_put.txt
    exit 1
fi
if ! grep -q "UpdatedIntegrationProduct" ./tmp6_put.txt; then
    echo "Test 6 failed - PUT body does not contain updated entity data"
    echo "Actual output:"
    cat ./tmp6_put.txt
    exit 1
fi

# 6d. GET again to verify the update was stored
echo "Verifying update via GET /api/Products/$ENTITY_ID..."
curl -s -i "localhost:80/api/Products/${ENTITY_ID}" > ./tmp6_get_updated.txt
CURL_EXIT=$?
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT during updated GET"
    exit 1
fi

if ! grep -q "200 OK" ./tmp6_get_updated.txt; then
    echo "Test 6 failed - updated GET did not return 200 OK"
    echo "Actual output:"
    cat ./tmp6_get_updated.txt
    exit 1
fi
if ! grep -qi "Content-Type: application/json" ./tmp6_get_updated.txt; then
    echo "Test 6 failed - updated GET missing application/json Content-Type"
    echo "Actual output:"
    cat ./tmp6_get_updated.txt
    exit 1
fi
if ! grep -q "UpdatedIntegrationProduct" ./tmp6_get_updated.txt; then
    echo "Test 6 failed - updated GET body does not contain updated entity data"
    echo "Actual output:"
    cat ./tmp6_get_updated.txt
    exit 1
fi

# 6e. GET list of IDs via GET /api/Products and verify that ID is present
echo "Listing entities via GET /api/Products..."
curl -s -i "localhost:80/api/Products" > ./tmp6_list.txt
CURL_EXIT=$?
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT during list GET"
    exit 1
fi

if ! grep -q "200 OK" ./tmp6_list.txt; then
    echo "Test 6 failed - list GET did not return 200 OK"
    echo "Actual output:"
    cat ./tmp6_list.txt
    exit 1
fi
if ! grep -qi "Content-Type: application/json" ./tmp6_list.txt; then
    echo "Test 6 failed - list GET missing application/json Content-Type"
    echo "Actual output:"
    cat ./tmp6_list.txt
    exit 1
fi
# Check that the response body contains the ID and looks like a JSON arr
if ! grep -q "\"${ENTITY_ID}\"" ./tmp6_list.txt; then
    echo "Test 6 failed - list GET body does not contain expected ID ${ENTITY_ID}"
    echo "Actual output:"
    cat ./tmp6_list.txt
    exit 1
fi

# 6f. Delete the entity via DELETE /api/Products/{id}
echo "Deleting entity via DELETE /api/Products/$ENTITY_ID..."
curl -s -i -X DELETE "localhost:80/api/Products/${ENTITY_ID}" > ./tmp6_delete.txt
CURL_EXIT=$?
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT during DELETE"
    exit 1
fi

# Check 200 OK and success message
if ! grep -q "200 OK" ./tmp6_delete.txt; then
    echo "Test 6 failed - DELETE did not return 200 OK"
    echo "Actual output:"
    cat ./tmp6_delete.txt
    exit 1
fi
if ! grep -q "Entity deleted successfully" ./tmp6_delete.txt; then
    echo "Test 6 failed - DELETE did not return expected success message"
    echo "Actual output:"
    cat ./tmp6_delete.txt
    exit 1
fi

# 6g. Verify the entity is no longer retrievable (GET should return 404)
echo "Verifying entity is no longer retrievable..."
curl -s -i "localhost:80/api/Products/${ENTITY_ID}" > ./tmp6_get2.txt
CURL_EXIT=$?
if [[ $CURL_EXIT -ne 0 ]]; then
    echo "Curl failed with exit code $CURL_EXIT during second GET"
    exit 1
fi

if grep -q "404 Not Found" ./tmp6_get2.txt; then
    echo "Test 6 passed."
    rm ./tmp6_post.txt ./tmp6_get.txt ./tmp6_delete.txt ./tmp6_get2.txt
else
    echo "Test 6 failed - second GET did not return 404 Not Found"
    echo "Actual output:"
    cat ./tmp6_get2.txt
    exit 1
fi

# --- Test 7: Malformed Request - Garbage Input ---
echo ""
echo "========== Test 7: Malformed Request - Garbage Input =========="

echo "Sending garbage input..."
printf "GARBAGE DATA RANDOM TEXT" | nc -C localhost 80 -q 0 > ./tmp7.txt

echo "Checking response..."
if grep -q "400 Bad Request" ./tmp7.txt; then
    rm ./tmp7.txt
    echo "Test 7 passed."
else
    echo "Test 7 failed - missing 400 Bad Request status"
    echo "Actual output:"
    cat ./tmp7.txt
    exit 1
fi

# --- Test 8: Malformed Request - Missing HTTP Version ---
echo ""
echo "========== Test 8: Malformed Request - Missing HTTP Version =========="

echo "Sending request without HTTP version..."
printf "GET /test\r\n\r\n" | nc -C localhost 80 -q 0 > ./tmp8.txt

echo "Checking response..."
if grep -q "400 Bad Request" ./tmp8.txt; then
    rm ./tmp8.txt
    echo "Test 8 passed."
else
    echo "Test 8 failed - missing 400 Bad Request status"
    echo "Actual output:"
    cat ./tmp8.txt
    exit 1
fi

# --- Test 9: Malformed Request - Invalid HTTP Method ---
echo ""
echo "========== Test 9: Malformed Request - Invalid HTTP Method =========="

echo "Sending request with invalid method..."
printf "INVALID /test HTTP/1.1\r\n\r\n" | nc -C localhost 80 -q 0 > ./tmp9.txt

echo "Checking response..."
if grep -q "400 Bad Request" ./tmp9.txt; then
    rm ./tmp9.txt
    echo "Test 9 passed."
else
    echo "Test 9 failed - missing 400 Bad Request status"
    echo "Actual output:"
    cat ./tmp9.txt
    exit 1
fi

# --- Test 10: Malformed Request - Invalid HTTP Version ---
echo ""
echo "========== Test 10: Malformed Request - Invalid HTTP Version =========="

echo "Sending request with HTTP/2.0..."
printf "GET /test HTTP/2.0\r\n\r\n" | nc -C localhost 80 -q 0 > ./tmp10.txt

echo "Checking response..."
if grep -q "400 Bad Request" ./tmp10.txt; then
    rm ./tmp10.txt
    echo "Test 10 passed."
else
    echo "Test 10 failed - missing 400 Bad Request status"
    echo "Actual output:"
    cat ./tmp10.txt
    exit 1
fi

# --- Test 11: Malformed Request - Path Without Leading Slash ---
echo ""
echo "========== Test 11: Malformed Request - Path Without Leading Slash =========="

echo "Sending request with path missing leading slash..."
printf "GET test HTTP/1.1\r\n\r\n" | nc -C localhost 80 -q 0 > ./tmp11.txt

echo "Checking response..."
if grep -q "400 Bad Request" ./tmp11.txt; then
    rm ./tmp11.txt
    echo "Test 11 passed."
else
    echo "Test 11 failed - missing 400 Bad Request status"
    echo "Actual output:"
    cat ./tmp11.txt
    exit 1
fi

# --- Test 12: Malformed Request - Path With Spaces ---
echo ""
echo "========== Test 12: Malformed Request - Path With Spaces =========="

echo "Sending request with spaces in path..."
printf "GET /test path HTTP/1.1\r\n\r\n" | nc -C localhost 80 -q 0 > ./tmp12.txt

echo "Checking response..."
if grep -q "400 Bad Request" ./tmp12.txt; then
    rm ./tmp12.txt
    echo "Test 12 passed."
else
    echo "Test 12 failed - missing 400 Bad Request status"
    echo "Actual output:"
    cat ./tmp12.txt
    exit 1
fi

# --- Cleanup: Remove temporary files ---
echo ""
echo "========== Cleanup =========="
rm -rf "$STATIC_DIR"
echo "Removed temporary static directory"

rm -rf "$CRUD_DIR"
echo "Removed temporary CRUD directory"

echo ""
echo "All tests passed successfully!"
exit 0



<<Comment
# Test 1: testing good request
echo "Test1: good request..."
echo "Start the server..."                                                                                                    │
../build/bin/server ./test_config &                                                                                           │
SERVER_PID=$!                                                                                                                 │
echo "Server started with PID: $SERVER_PID"                                                                                   │
sleep 1            
                                                                                                           │                                                                                                                               │
echo "Sending request..." 
curl -s localhost:80 > ./tmp.txt

echo "Shutting down server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Compare the result"
if cmp -s ./expected.txt ./tmp.txt ; then
    rm ./tmp.txt
    echo -e "Test 1 pass"
else
    # failing? check tmp
    echo "Actual output:"
    cat ./tmp.txt
    # rm ./tmp.txt
    echo -e "Test 1 fail - content not same"
    exit 1
fi


# Test 2: testing bad request
echo "Test2: bad request..."
echo "Start the server and sending request..."
timeout 0.2s ../build/bin/server ../configs/server_config &> /dev/null &
sleep 0.1
cat ./bad_request.txt | nc -C localhost 80 -q 0 > ./tmp2.txt
sleep 0.1
echo "Compare the result"
if cmp -s ./expected_bad.txt ./tmp2.txt ; then
    rm ./tmp2.txt
    echo -e "Test 2 pass"
else
    # failing? check tmp2
    echo "Actual output:"
    cat ./tmp2.txt
    # rm ./tmp2.txt
    echo -e "Test 2 fail - content not same"
    exit 1
fi
Comment
