all: librtMessaging.so rtrouted sample_send sample_recv rtsub sample_provider

CC=gcc

RTMSG_SRCS=\
  rtLog.c \
  rtBuffer.c \
  rtError.c \
  rtEncoder.c \
  rtSocket.c \
  rtConnection.c \
  rtMessageHeader.c \
  rtDebug.c \
  rtMessage.c \
  rtDataModel.c \
  rtVector.c

RTROUTER_SRCS=rtrouted.c

ifeq ($V, 1)
  CC_PRETTY = $(CC)
  LD_PRETTY = $(CC)
  CC_PRETTY = $(CC)
  BUILD_CC_PRETTY = $(BUILD_CC)
else
  LD_PRETTY = @echo "[LINK] $@" ; $(CXX)
  CC_PRETTY = @echo " [CC] $<" ; $(CC)
  BUILD_CC_PRETTY = @echo " [CC] $<" ; $(BUILD_CXX)
endif

ifeq ($(DEBUG), 1)
  CFLAGS += -g -fno-inline -O0 -DRT_DEBUG -IcJSON
else
  CFLAGS += -O2
endif

ifeq ($(PROFILE), 1)
  CFLAGS += -pg
endif

#CFLAGS+=-Werror -Wall -Wextra -DRT_PLATFORM_LINUX -I. -fPIC
CFLAGS+=-Wall -Wextra -DRT_PLATFORM_LINUX -I. -fPIC
LDFLAGS=-L. -pthread
OBJDIR=obj

RTMSG_OBJS:=$(patsubst %.c, %.o, $(notdir $(RTMSG_SRCS)))
RTMSG_OBJS:=$(patsubst %, $(OBJDIR)/%, $(RTMSG_OBJS))

$(OBJDIR)/%.o : %.c
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CC_PRETTY) -c $(CFLAGS) $< -o $@

debug:
	@echo $(OBJS)

clean:
	rm -rf obj *.o librtMessaging.so
	rm -f rtrouted
	rm -f sample_send sample_recv sample_provider

librtMessaging.so: $(RTMSG_OBJS)
	$(CC_PRETTY) $(RTMSG_OBJS) $(LDFLAGS) -LcJSON -lcjson -shared -o $@

rtrouted: rtrouted.c
	$(CC_PRETTY) $(CFLAGS) rtrouted.c -o rtrouted -L. -lrtMessaging -LcJSON -lcjson

sample_send: librtMessaging.so sample_send.c
	$(CC_PRETTY) $(CFLAGS) sample_send.c -L. -lrtMessaging -o sample_send -LcJSON -lcjson

sample_recv: librtMessaging.so sample_recv.c
	$(CC_PRETTY) $(CFLAGS) sample_recv.c -L. -lrtMessaging -o sample_recv -LcJSON -lcjson

sample_provider: librtMessaging.so sample_provider.c
	$(CC_PRETTY) $(CFLAGS) sample_provider.c -L. -lrtMessaging -o sample_provider -LcJSON -lcjson

rtsub: librtMessaging.so rtsub.c
	$(CC_PRETTY) $(CFLAGS) rtsub.c -L. -lrtMessaging -o rtsub -LcJSON -lcjson
