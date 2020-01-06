#!/bin/bash

set -e 
export PATH=$HOME/.local/bin:$PATH
if [ -z $COV_DIR ]
then
	COV_DIR=/remote/us01dwt1p068/cov-analysis-linux64-2019.06
fi

CHECK_OPTS="--checker-option UNINIT_CTOR:ignore_empty_constructors"
DISABLE_CHECKERS="--disable PASS_BY_VALUE --disable SWAPPED_ARGUMENTS --disable MISSING_BREAK"
P4PORT="p4p-us01:1900"
RES_DIR="coverity_results"
COMMAND=""
MISRA_CONFIG=""
EMIT_INFO=""
COMPILER_CONFIG=""
COMPILER=""
BUILD_ONLY=0
EXIT_ON_FAILURE=0
RETURN_ERROR_COUNT=0
#Default sream is driver_llvm, now driver_llvm , and llvm available
STREAM=embARC_MW_BSP
PROJECT=embARC_MW
if [ ! -z ${WORKSPACE} ]
then
	PREFIX_PATH=${WORKSPACE}
else
    PREFIX_PATH=$(pwd)
fi

while [ $# -ne 0 ]; do
    case $1 in
        -resdir)
            shift;
            RES_DIR=${1}
            ;;
        -build_only)
            BUILD_ONLY=1
            ;;
        -exit_on_failure)
            EXIT_ON_FAILURE=1
            ;;
        -return_error_count)
            RETURN_ERROR_COUNT=1
            ;;
	    -stream)
            shift;
            STREAM=${1}
            ;;
        -project)
            shift;
            PROJECT=${1}
            ;;
	   -remove-prefix)
            shift;
            PREFIX_PATH=${1}
            ;;
       -misra-conf)
            shift;
            MISRA_CONFIG="--coding-standard-config ${1}"
            EMIT_INFO="--emit-complementary-info"
            ;;
       -compiler-conf)
            shift;
            COMPILER_CONFIG="-c $1 "
            ;;
        -compiler)
            shift;
            COMPILER="$1"
            ;;
        *)
            COMMAND="${COMMAND} ${1}"
            ;;
    esac
    shift
done

echo "========Starting running coverity on stream: ${STREAM}"

LOGFILE=${RES_DIR}/run_coverity.log
mkdir -vp ${RES_DIR}

>$LOGFILE
exec >  >(tee -a $LOGFILE)
exec 2> >(tee -a $LOGFILE >&2)

# Run build command under Coverity
#

if [ ! -z "${COMPILER}" ]
then
    echo "${COV_DIR}/cov-configure"
    ${COV_DIR}/bin/cov-configure -c config.xml -co ccac  -p metawarecc:ccac --template \
    --xml-option=append_arg:--ppp_translator \
    --xml-option=append_arg:"replace/_Pragma\(\"clang(\s)*loop(\s)*pipeline_options\(0x\d*\)\"\)/" \
    --xml-option=append_arg@:"-Dext_vector_type=vector_size"
    ${COV_DIR}/bin/cov-build -c config.xml ${EMIT_INFO}  -dir ${RES_DIR} --tmpdir ${RES_DIR}/tmp  ${COMPILER_CONFIG} ${COMMAND}
else
    ${COV_DIR}/bin/cov-build -dir ${RES_DIR} --initialize
    ${COV_DIR}/bin/cov-build ${EMIT_INFO} -dir ${RES_DIR} --capture  --tmpdir ${RES_DIR}/tmp ${COMPILER_CONFIG} ${COMMAND}
    ${COV_DIR}/bin/cov-build --dir ${RES_DIR} --finalize
    ${COV_DIR}/bin/cov-manage-emit --dir ${RES_DIR} add-other-hosts 
    if [ -f ${RES_DIR}/output/emit-db-cache.lock ]
    then
        rm -rf ${RES_DIR}/output/emit-db-cache.lock
    fi
fi

EXIT_CODE=${?}
if (( ( "${EXIT_ON_FAILURE}" == "1" ) && ( "${EXIT_CODE}" != "0") )); then
  exit ${EXIT_CODE};
fi

if [ ${BUILD_ONLY} -eq 1 ]; then
  exit 0;
fi
# Analyze the results
#
PROC_NUM=$(cat /proc/cpuinfo | grep processor | wc -l)
${COV_DIR}/bin/cov-analyze -j $PROC_NUM --allow-unmerged-emits ${CHECK_OPTS} ${DISABLE_CHECKERS} -dir ${RES_DIR} --tmpdir ${RES_DIR}/tmp  --strip-path ${PREFIX_PATH} ${MISRA_CONFIG} || echo "There is nothing new build" 

EXIT_CODE=${?}
if (( ( "${EXIT_ON_FAILURE}" == "1" ) && ( "${EXIT_CODE}" != "0") )); then
  exit ${EXIT_CODE};
fi

# Format the Coverity errors, excluding errors in following 3rd party components:
#   - optionparser.h              (Lean Mean C++ Option Parser)
#   - */include/llvm/*            (LLVM headerfiles)
#   - */Gtest/*                   (GoogleTest Unit test framework)
#   - */mwelf/*                   (MetaWare ELF loader)
#   - */tinyxml2/*                (TinyXML2 XML parser)
#   - */softfloat/*               (SoftFloat floating point lib)
#   - */gcc-4.8.1/*               (GCC headerfiles)
#   - */include/tlm_utils/*       (SC 2.3.0 TLM header files)
#  

if [ -z "$JENKINS_URL" ]
then 
    rm -rf ${RES_DIR}/html_previous
    if [ -d ${RES_DIR}/html ]
    then
        mv ${RES_DIR}/html ${RES_DIR}/html_previous  >& /dev/null        
    fi
    ${COV_DIR}/bin/cov-format-errors --dir ${RES_DIR} --tmpdir ${RES_DIR}/tmp --html-output ${RES_DIR}/html -x -X \
   --exclude-files '(${WORKSPACE}/tests/|${WORKSPACE}/options/)'
    EXIT_CODE=${?}
    if (( ( "${EXIT_ON_FAILURE}" == "1" ) && ( "${EXIT_CODE}" != "0") )); then
    exit ${EXIT_CODE};
    fi

    ERROR_COUNT=$(grep -E "Processed .+ C/C\+\+ errors?\." ${LOGFILE} | awk '{print $2}')
    if [ ${RETURN_ERROR_COUNT} -eq 1 ]; then
    exit $(( ${ERROR_COUNT} ));
    fi

else
    if [ -z ${PREFIX_PATH} ]
    then
        echo "cov-commit-defects need parameter of --strip-path"
        exit -1
    fi
    echo "Commit coverity data"
    ${COV_DIR}/bin/cov-commit-defects --dir ${RES_DIR} --host "us01-arc-coverity8"  --stream ${STREAM} --user "${AUTO_USER}"  --password "${AUTO_PASSWORD}" --description "changelist ${P4_CHANGELIST} from ${BUILD_URL}" 
fi

exit 0;
