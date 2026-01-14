### Build/test container ###
# Define builder stage
FROM d20:base AS builder

# Share work directory
COPY . /usr/src/project
WORKDIR /usr/src/project/build

# Build and test
RUN cmake ..
RUN make
RUN ctest --output-on-failure


### Deploy container ###
# Define deploy stage
FROM ubuntu:noble AS deploy

COPY --from=builder /usr/src/project/build/bin/server .
COPY --from=builder /usr/src/project/static_content/ /app/static/
COPY --from=builder /usr/src/project/configs/server_config .

EXPOSE 80

# Use ENTRYPOINT to specify the binary name
ENTRYPOINT ["./server"]

# Use CMD to specify arguments to ENTRYPOINT
CMD ["server_config"]

