sed -i '/collection/d' .gitignore
git rm -r --cached library source tests
git rm --cached .gitignore \*.sh \*.txt \*.md .clang-format
echo -e "\n/library\n/source\ntests\n.clang-format\n*.sh\n*.txt\n*.md\n" >> .gitignore
./packup.sh
git add .
git commit -m "collect commit files"