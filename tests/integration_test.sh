#!/bin/bash

SIMPLECONTAINER="../simplecontainer"


GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'


error_exit() {
    echo -e "${RED}خطا: $1${NC}" >&2
    exit 1
}


run_test() {
    TEST_NAME=$1
    shift
    echo "در حال اجرای تست: $TEST_NAME"
    
    
    OUTPUT=$("$@" 2>&1)
    RESULT=$?
    
    if [ $RESULT -eq 0 ]; then
        echo -e "${GREEN}✓ تست با موفقیت انجام شد${NC}"
    else
        echo -e "${RED}✗ تست با خطا مواجه شد (کد خروج: $RESULT)${NC}"
        echo -e "${RED}خروجی:${NC}"
        echo "$OUTPUT"
        return 1
    fi
    
    return 0
}


if [ ! -f "$SIMPLECONTAINER" ]; then
    error_exit "فایل اجرایی $SIMPLECONTAINER پیدا نشد"
fi


if [ "$(id -u)" -ne 0 ]; then
    error_exit "این اسکریپت باید با دسترسی root اجرا شود"
fi

echo "شروع آزمون‌های یکپارچگی..."


run_test "Hello World" $SIMPLECONTAINER run --name test_hello ../examples/hello


run_test "محدودیت حافظه" $SIMPLECONTAINER run --name test_memory --memory 50M ../examples/resource_test --memory-test || true


run_test "تخصیص CPU" $SIMPLECONTAINER run --name test_cpu --cpu 0 ../examples/resource_test --cpu-test


run_test "محدودیت I/O" $SIMPLECONTAINER run --name test_io --io-weight 50 ../examples/resource_test --io-test


run_test "لیست کانتینرها" $SIMPLECONTAINER list


CONTAINER_ID=$($SIMPLECONTAINER list | grep test_hello | awk '{print $1}')


if [ -n "$CONTAINER_ID" ]; then
    run_test "وضعیت کانتینر" $SIMPLECONTAINER status $CONTAINER_ID
else
    echo -e "${RED}نمی‌توان شناسه کانتینر را دریافت کرد${NC}"
fi


run_test "اجرای shell" $SIMPLECONTAINER run --name test_shell --detach /bin/sleep 10


SHELL_ID=$($SIMPLECONTAINER list | grep test_shell | awk '{print $1}')
if [ -n "$SHELL_ID" ]; then
    run_test "توقف کانتینر" $SIMPLECONTAINER stop $SHELL_ID
else
    echo -e "${RED}نمی‌توان شناسه کانتینر shell را دریافت کرد${NC}"
fi

echo "آزمون‌های یکپارچگی به پایان رسید"