// start env
bin/server ../configs/server_config

// build base
docker build -t d20:base -f docker/base.Dockerfile .

// build image
docker build -t d20:latest -f docker/Dockerfile .

// coverage image
docker build -t d20:coverage -f docker/coverage.Dockerfile .

// start server
docker run -p 8080:80 --rm d20:latest

// on an outside terminal, run:
curl -v localhost:8080


// list running containers
docker ps
// stop docker container
docker stop <container_id>

