## Documentation project

### Installation

	sudo apt-get install pandoc inotify-tools

### Usage

The documentation is written in [Pandoc's
Markdown](http://johnmacfarlane.net/pandoc/README.html#pandocs-markdown)

After that, make a PDF by running `make`. This process can be automated by
running `build-forever.sh` script that detects when any `*.md` file is
changed.
