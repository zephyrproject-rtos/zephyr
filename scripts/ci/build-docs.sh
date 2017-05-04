#!/bin/bash
set -e

echo "- Install dependencies"
sudo apt-get install doxygen make
sudo pip install breathe sphinx awscli sphinx_rtd_theme

cd ${MAIN_REPO_STATE}
source zephyr-env.sh

cp -a /build/IN/docs_theme_repo/gitRepo doc/themes/zephyr-docs-theme
ls -la doc/themes

echo "- Building docs..."
make DOC_TAG=daily htmldocs > doc.log 2>&1
echo "- Uploading to AWS S3..."
aws s3 sync --quiet --delete doc/_build/html s3://zephyr-docs/online/dev

echo "Done"
