
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src modules po docs
dist_doc_DATA = README 
EXTRA_DIST = @PACKAGE_NAME@.spec.in monitor.cfg httpd.conf.patch

DISTCLEANFILES = @PACKAGE_NAME@.spec

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

sysconfigdir = /etc

sysconfig_DATA = monitor.cfg

test : $(top_builddir)/src/monitor /tmp/${prefix}/share/locale/
	$(top_builddir)/src/monitor --module_dir=$(top_builddir)/modules  --config=$(top_srcdir)/monitor.cfg --locale=/tmp/${prefix}/share/locale/  ${ARG}

/tmp/${prefix}/share/locale/ : po/zh_CN.gmo
	cd po && $(MAKE) DESTDIR=/tmp install

po/zh_CN.gmo: po/zh_CN.po
	cd po && $(MAKE) zh_CN.gmo
	
$(top_builddir)/src/monitor :
	cd src && $(MAKE)

gdb : $(top_builddir)/src/monitor /tmp/${prefix}/share/locale/
	gdbserver localhost:5000 $(top_builddir)/src/monitor --module_dir=$(top_builddir)/modules  --config=$(top_srcdir)/monitor.cfg --locale=/tmp/${prefix}/share/locale/  ${ARG}
			
install-data-hook: httpd.conf.patch
	if [ -x ${DESTDIR}/etc/apache ] ; then \
	cat  $< | (  cd ${DESTDIR}/etc/apache/ && patch )\
fi
