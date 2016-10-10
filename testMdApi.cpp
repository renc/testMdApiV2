// testTraderApi.cpp : 定义控制台应用程序的入口点。
//

#include <cstdio>
#include <iostream>
#include ".\v6.3.6_20150515_traderapi_win64\tradeapidll\ThostFtdcMdApi.h"

// UserApi对象
CThostFtdcMdApi* pUserApi;

// 配置参数
//char FRONT_ADDR[] = "tcp://asp-sim2-md1.financial-trading-platform.com:26213";		// 前置地址
//char FRONT_ADDR[] = "tcp://180.166.80.97:41213";	//中金所CFFEX
char FRONT_ADDR[] = "tcp://180.168.146.187:10010";
TThostFtdcBrokerIDType	BROKER_ID = "9999";				// 经纪公司代码(需要联系一家期货公司的技术提供给你)
TThostFtdcInvestorIDType INVESTOR_ID = "067906";//can.  //"066718";//canis.			// 投资者代码(需要联系期货公司技术提供)
TThostFtdcPasswordType  PASSWORD = "123456";			// 用户密码(需要联系期货公司技术提供)
char *ppInstrumentID[] = {"FG609"/*, "ru1301"*/};			// 行情订阅列表(需要设置正在发布行情的合约过期合约没用行情)
int iInstrumentID = 1;									// 行情订阅数量
//说明：上述参数因期货公司不同而不同，这个不难，你联系一家期货公司技术就说搞CTP程序化交易，人家会提供给你
//参数已设置，在VS2012下一编译，你就看到忽忽的行情出来了啊
//真搞不定有可以联系我，bj9595@qq.com
int iRequestID = 0;// 请求编号

// renc: to test every interface of CThostFtdcMdSpi 
class CMdSpi : public CThostFtdcMdSpi
{
public: // renc: interfaces from  CThostFtdcMdSpi. 

	///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
	virtual void OnFrontConnected();

	///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
	///@param nReason 错误原因
	///        0x1001 网络读失败
	///        0x1002 网络写失败
	///        0x2001 接收心跳超时
	///        0x2002 发送心跳失败
	///        0x2003 收到错误报文
	virtual void OnFrontDisconnected(int nReason);

	///心跳超时警告。当长时间未收到报文时，该方法被调用。
	///@param nTimeLapse 距离上次接收报文的时间
	virtual void OnHeartBeatWarning(int nTimeLapse);

	///登录请求响应
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	///登出请求响应
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///错误应答
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast);
	
	///订阅行情应答
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///取消订阅行情应答
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///订阅询价应答
	virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

	///深度行情通知
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

	///询价通知
	virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp);
private:
	void ReqUserLogin();
	void SubscribeMarketData();
	
	// 
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
};

void CMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast)
{
	std::cerr << "--->>> "<< __FUNCTION__ << std::endl;
	IsErrorRspInfo(pRspInfo);
}

void CMdSpi::OnFrontDisconnected(int nReason)
{
	std::cerr << "--->>> " << __FUNCTION__ << std::endl;
	std::cerr << "--->>> Reason = " << nReason << std::endl;
}
		
void CMdSpi::OnHeartBeatWarning(int nTimeLapse)
{
	std::cerr << "--->>> " << __FUNCTION__ << std::endl;
	std::cerr << "--->>> nTimerLapse = " << nTimeLapse << std::endl;
}

void CMdSpi::OnFrontConnected()
{
	std::cerr << "--->>> " << __FUNCTION__ << std::endl;
	///用户登录请求
	ReqUserLogin(); // renc: not a virtual function.
}

void CMdSpi::ReqUserLogin()
{
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strcpy_s(req.BrokerID, sizeof(TThostFtdcBrokerIDType), BROKER_ID);
	strcpy_s(req.UserID, sizeof(TThostFtdcUserIDType), INVESTOR_ID);
	strcpy_s(req.Password, sizeof(TThostFtdcPasswordType), PASSWORD);
	int iResult = pUserApi->ReqUserLogin(&req, ++iRequestID);
	std::cerr << u8"--->>> send user login request: " << ((iResult == 0) ? u8"succeed" : u8"fail") << std::endl;
}

void CMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	std::cerr << "--->>> " << __FUNCTION__ << std::endl;
	if (bIsLast && !IsErrorRspInfo(pRspInfo))
	{
		///获取当前交易日
		std::cerr << u8"--->>> GetTradingDay = " << pUserApi->GetTradingDay() << std::endl;
		// 请求订阅行情
		SubscribeMarketData();	
	}
}

void CMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	std::cerr << "--->>> " << __FUNCTION__ << std::endl;
}

void CMdSpi::SubscribeMarketData()
{
	int iResult = pUserApi->SubscribeMarketData(ppInstrumentID, iInstrumentID);//订阅行情。
	std::cerr << u8"--->>> SubscribeMarketData: " << ((iResult == 0) ? "Succeed" : "Fail") << std::endl;

	iResult = pUserApi->SubscribeForQuoteRsp(ppInstrumentID, iInstrumentID); //订阅询价。
	std::cerr << "--->> SubscribeForQuotoRsp: " << ((iResult == 0) ? "Succeed" : "Fail") << std::endl;
}

void CMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	std::cerr << __FUNCTION__ << std::endl;
}

void CMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	std::cerr << __FUNCTION__ << std::endl;
}


void CMdSpi::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	std::cerr << __FUNCTION__ << std::endl;

}

void CMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	// renc: quotation return. 
	std::cerr << __FUNCTION__ << std::endl;
	std::cerr << u8"交易日: " << pDepthMarketData->TradingDay << std::endl;
	std::cerr << u8"合约代码: " << pDepthMarketData->InstrumentID << std::endl;
	std::cerr << u8"交易所代码: " << pDepthMarketData->ExchangeID << std::endl;
	std::cerr << u8"合约在交易所的代码: " << pDepthMarketData->ExchangeInstID << std::endl;
	std::cerr << u8"最新价: " << pDepthMarketData->LastPrice << std::endl;
	std::cerr << u8"上次结算价: " << pDepthMarketData->PreSettlementPrice << std::endl;
	std::cerr << u8"昨收盘: " << pDepthMarketData->PreClosePrice << std::endl;
	std::cerr << u8"昨持仓量: " << pDepthMarketData->PreOpenInterest << std::endl;
	std::cerr << u8"今开盘(价): " << pDepthMarketData->OpenPrice << std::endl;
	std::cerr << u8"最高价: " << pDepthMarketData->HighestPrice << std::endl;
	std::cerr << u8"最低价: " << pDepthMarketData->LowestPrice << std::endl;
	std::cerr << u8"数量: " << pDepthMarketData->Volume << std::endl;
	std::cerr << u8"成交金额: " << pDepthMarketData->Turnover << std::endl;
	std::cerr << u8"持仓量: " << pDepthMarketData->OpenInterest << std::endl;
	std::cerr << "..." << std::endl;
	std::cerr << u8"申买价一: " << pDepthMarketData->BidPrice1 << std::endl;
	std::cerr << u8"申买量一: " << pDepthMarketData->BidVolume1 << std::endl;
	std::cerr << u8"申卖价一: " << pDepthMarketData->AskPrice1 << std::endl;
	std::cerr << u8"申卖量一: " << pDepthMarketData->AskVolume1 << std::endl;
	std::cerr << u8"当日均价: " << pDepthMarketData->AveragePrice << std::endl;
	std::cerr << "End of " << __FUNCTION__ << std::endl;
}

void CMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{
	std::cerr << __FUNCTION__ << std::endl;

}
bool CMdSpi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
	// 如果ErrorID != 0, 说明收到了错误的响应
	bool bResult = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if (bResult)
		std::cerr << "--->>> ErrorID=" << pRspInfo->ErrorID << ", ErrorMsg=" << pRspInfo->ErrorMsg << std::endl;
	return bResult;
}

void main(void)
{
	// 初始化UserApi
	pUserApi = CThostFtdcMdApi::CreateFtdcMdApi();			// 创建UserApi
	CThostFtdcMdSpi* pUserSpi = new CMdSpi();
	pUserApi->RegisterSpi(pUserSpi);						// 注册事件类
	pUserApi->RegisterFront(FRONT_ADDR);					// connect
	pUserApi->Init();
	pUserApi->Join();
//	pUserApi->Release();


	std::cout << "To Exit ...\n";
	getc(stdin);
}

/*
第一套：
标准CTP：
第一组：Trade Front：180.168.146.187:10000，Market Front：180.168.146.187:10010；【电信】
第二组：Trade Front：180.168.146.187:10001，Market Front：180.168.146.187:10011；【电信】
第三组：Trade Front：218.202.237.33 :10002，Market Front：218.202.237.33 :10012；【移动】
交易阶段(服务时间)：与实际生产环境保持一致

CTPMini1：
第一组：Trade Front：180.168.146.187:10003，Market Front：180.168.146.187:10013；【电信】

第二套：
交易前置：180.168.146.187:10030，行情前置：180.168.146.187:10031；【7x24】
第二套环境仅服务于CTP API开发爱好者，仅为用户提供CTP API测试需求，不提供结算等其它服务。
新注册用户，需要等到第二个交易日才能使用第二套环境。
账户、钱、仓跟第一套环境上一个交易日保持一致。
交易阶段(服务时间)：交易日，16：00～次日09：00；非交易日，16：00～次日15：00。
用户通过SimNow的账户（上一个交易日之前注册的账户都有效）接入环境，建议通过商业终端进行模拟交易的用户使用第一套环境。

*/