
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src modules po
dist_doc_DATA = README
EXTRA_DIST = @PACKAGE_NAME@-hotel.spec.in @PACKAGE_NAME@-cafe.spec.in

DISTCLEANFILES = @PACKAGE_NAME@-hotel.spec @PACKAGE_NAME@-cafe.spec

if	ENABLE_HOTEL			
	spec = 	@PACKAGE_NAME@-hotel.spec
else
	spec = 	@PACKAGE_NAME@-cafe.spec 
endif

rpm: dist @PACKAGE_NAME@-hotel.spec @PACKAGE_NAME@-cafe.spec
	rpmbuild -bb \
			--define "_sourcedir `pwd`" \
			--define "_builddir `pwd`" \
			--define "_specdir `pwd`" \
			--define "_rpmdir `pwd`" \
			--define "_srcrpmdir `pwd`" \
			$(spec)

srpm: dist @PACKAGE_NAME@.spec
	rpmbuild -bs \
			--define "_sourcedir `pwd`" \
			--define "_builddir `pwd`" \
			--define "_srcrpmdir `pwd`" \
		    --define "_rpmdir `pwd`" \
			--define "_specdir `pwd`" \
			$(spec)
