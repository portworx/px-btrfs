#!/bin/sh 

KSRC=$1
if [ ! -f ${KSRC}/include/generated/uapi/linux/version.h ]; then
	echo "Could not determine the kernel version"
else
	RHEL_RELEASE=$(sed -rn 's/^#define RHEL_RELEASE "(.*)"/\1/p' ${KSRC}/include/generated/uapi/linux/version.h)
	RHEL_RELEASE_MAJOR=$(echo ${RHEL_RELEASE} | awk 'BEGIN {FS="."}{print $1}')
	RHEL_RELEASE_MINOR=$(echo ${RHEL_RELEASE} | awk 'BEGIN {FS="."}{print $2}')
	RHEL_RELEASE_PATCH=$(echo ${RHEL_RELEASE} | awk 'BEGIN {FS="."}{print $3}')
                                                                                
#echo ${RHEL_RELEASE}
#echo ${RHEL_RELEASE_MAJOR}
#echo ${RHEL_RELEASE_MINOR}
#echo ${RHEL_RELEASE_PATCH}

	if [ RHEL_RELEASE_MAJOR != "" ]; then
		RHEL_VERSION_CODE=$((RHEL_RELEASE_MAJOR * 10000))
		if [ RHEL_RELEASE_MINOR != "" ]; then
			RHEL_VERSION_CODE=$((RHEL_VERSION_CODE + RHEL_RELEASE_MINOR * 100))
			if [ RHEL_RELEASE_PATCH != "" ]; then
				RHEL_VERSION_CODE=$((RHEL_VERSION_CODE + RHEL_RELEASE_PATCH))
			fi
		fi
	fi
fi                                                                              
                                                                                
echo ${RHEL_VERSION_CODE} 

