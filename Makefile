
###
### Make to manipulate webpages.
### Note that code relies on MinifyAll extension to first minify the raw source

all: include/pages/RssMatrix.css.h \
	include/pages/update.html.h 
	
	@exec true 



include/pages/RssMatrix.css.h: WEBsources/RssMatrix-min.css
	@./ProduceDefine.py $< text/css  > $@

include/pages/update.html.h: WEBsources/update.html
	@./ProduceDefine.py $< text/html\;charset=UTF-8 > $@


clean:
	rm include/pages/*

