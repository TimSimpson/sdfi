cd /word_max

sudo apt-get install libboost1.58-all-dev
sudo apt-get install cmake

mkdir -p /word_max/books
cd /word_max/books

wget http://homepages.ihug.co.nz/~leonov/shakespeare.tar.bz2
tar xvf shakespeare.tar.bz2
rm shakespeare.tar.bz2

wget http://www.gutenberg.org/cache/epub/10/pg10.txt
mv pg10.txt king_james_bible.txt
