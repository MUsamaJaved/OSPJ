FLAGS=	--latex-engine=xelatex \
		--template=template.latex \
		--smart \
		--toc \
		--toc-depth=2 \
		--normalize \
		--variable=documentclass:report \
		--listings
		# --number-sections \
		# --highlight-style=tango \
		# --no-tex-ligatures \

all: *.md template.latex
	pandoc $(FLAGS) \
		-o documentation.tex \
		*.md
	pandoc $(FLAGS) \
		-o documentation.pdf \
		*.md

clean:
	rm -f *.pdf documentation.tex
