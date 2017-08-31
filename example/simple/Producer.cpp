// Producer.cpp : 定义控制台应用程序的入口点。
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <string>
#include <iostream>
#include <vector>

#ifdef WIN32
#   include <sys/timeb.h>
#   include <process.h>
#else
#   include <unistd.h>
#   include <sys/types.h>
#   include <signal.h>
#endif

#include <DefaultMQProducer.h>
#include <Message.h>
#include <SendResult.h>
#include <MQClientException.h>

void MySleep(long millis)
{

#ifdef WIN32
	::Sleep(millis);
#else
	struct timespec tv;
	tv.tv_sec = millis / 1000;
	tv.tv_nsec = (millis % 1000) * 1000000;
	 nanosleep(&tv, 0);
#endif
}

void Usage(const char* program)
{
	printf("Usage:%s ip:port [-n] [-v] [order]\n",program);
	printf("\t -n message count\n");
	printf("\t -v message size \n");
}

int main(int argc, char* argv[])
{
	if (argc<2)
	{
		Usage(argv[0]);
		return 0;
	}

	int count = 3;
	int size = 50;
	bool bOrderProducer = false;

	for (int i=2; i< argc; i++)
	{
		if (strcmp(argv[i],"-n")==0)
		{
			if (i+1 < argc)
			{
				count = atoi(argv[i+1]);
				i++;
			}
			else
			{
				Usage(argv[0]);
				return 0;
			}
		}
		else if (strcmp(argv[i],"-v")==0)
		{
			if (i+1 < argc)
			{
				size = atoi(argv[i+1]);
				i++;
			}
			else
			{
				Usage(argv[0]);
				return 0;
			}
		}
		else if(strcasecmp(argv[i], "order") == 0)
        {
            bOrderProducer = true;
        }
		else
		{
			Usage(argv[0]);
			return 0;
		}
	}


	DefaultMQProducer producer("MyProducerGroup");
	producer.setNamesrvAddr(argv[1]);
	producer.start();

	std::string tags[] = { "TagA", "TagB", "TagC", "TagD", "TagE" };

	MessageQueue* mq = NULL;
	char key[30];
    char szTime[20];
	char* value = new char[size];


    struct tm tmNow;
    time_t tNow = time(NULL);
    strftime(szTime,sizeof(szTime),"%Y%m%d%H%M%S",localtime_r(&tNow, &tmNow));
    
	sprintf(value,"Hello RocketMQ[%s]", szTime);
    size_t iLen = strlen(value);

	if (bOrderProducer) 
	{
		// choose only one queue for order message
		mq = new MessageQueue();
	    std::vector<MessageQueue>* mqs = producer.fetchPublishMessageQueues("CCR");
		std::vector<MessageQueue>::iterator it = mqs->begin();
		if (it!=mqs->end())
		{
			*mq = *it;
			printf("Producer order message: topic[%s], broker queue[%s,%d]\n",
				mq->getTopic().c_str(), mq->getBrokerName().c_str(), mq->getQueueId());
		}
		else 
		{
		    std::cout<<"No MessageQueue for topic[CCR]"<<std::endl;
			producer.shutdown();
			return -1;
		}
	}
	

	for (int i = 0; i < count; i++) {
		try
		{
			sprintf(key,"KEY[%s]%d", szTime, i);
			sprintf(value + iLen,"%d",i);
			//Message msg("TopicTest",// topic
			Message msg("CCR",// topic
				tags[i % 5],// tag
				key,// key
				value,// body
				strlen(value)+1);
			SendResult sendResult;
			if (mq != NULL )
			{
			    sendResult = producer.send(msg, *mq);
			}
			else 
			{
			    sendResult = producer.send(msg);
			}
			MySleep(100);
			sleep(5);
			printf("sendresult=%d,msgid=%s,queue=%d,value=%s\n",
                sendResult.getSendStatus(),sendResult.getMsgId().c_str(),
                sendResult.getMessageQueue().getQueueId(),value);
		}
		catch (MQClientException& e) {
			std::cout<<e<<std::endl;
			MySleep(3000);
		}
		catch (MQBrokerException& e) {
            std::cout<<e<<std::endl;
            MySleep(3000);
	    }
	}

	producer.shutdown();
	return 0;
}

