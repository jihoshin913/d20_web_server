echo "Authenticating"
gcloud auth configure-docker gcr.io
echo "Finished Authenticating"

echo "Pulling latest"
docker pull gcr.io/d20a1-475111/d20:latest
echo "Finished pulling latest"

echo "Applying tag"
docker tag gcr.io/d20a1-475111/d20:latest gcr.io/d20a1-475111/d20:release
echo "Finished applying tag"

echo "Pushing"
docker push gcr.io/d20a1-475111/d20:release
echo "Finished pushing"

echo "Done"