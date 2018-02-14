PROGRAM=PlaneGPS

EXTRA_CFLAGS=-DLWIP_HTTPD_CGI=1 -DLWIP_HTTPD_SSI=1 -I./fsdata

EXTRA_COMPONENTS=extras/gyneo6 extras/mbedtls extras/httpd

include ../../common.mk

html:
	@echo "Generating fsdata.."
	cd fsdata && ./makefsdata
