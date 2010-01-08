

SUBDIRS = libdreamtop monitor modules 
dist_doc_DATA = README
EXTRA_DIST = monitor-hotel.spec monitor-cafe.spec


buildrpms: dist-bzip2 monitor-hotel.spec monitor-cafe.spec
	mkdir -p ~/rpmbuild/SOURCES/
	mv monitor-*.tar.bz2 ~/rpmbuild/SOURCES/
	
if	ENABLE_HOTEL
	if [ "" = "$(TARGET)" ] ;then \
	QA_RPATHS=02 rpmbuild -bb $(top_srcdir)/monitor-hotel.spec ; \
	else QA_RPATHS=02 rpmbuild -bb $(top_srcdir)/monitor-hotel.spec --target=$(TARGET) ; fi
else
	if [ "" = "$(TARGET)" ] ;then \
	QA_RPATHS=02 rpmbuild -bb $(top_srcdir)/monitor-cafe.spec ; \
	else QA_RPATHS=02 rpmbuild -bb $(top_srcdir)/monitor-cafe.spec --target=$(TARGET) ; fi
endif
		
	mv ~/rpmbuild/RPMS/*/monitor-*$(TARGET).rpm ./
	rm ~/rpmbuild/SOURCES/monitor*