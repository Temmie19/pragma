#include "stdafx_client.h"
#include "pragma/gui/wiscrollcontainer.h"
#include <wgui/types/wiscrollbar.h>
#include <mathutil/umath.h>

LINK_WGUI_TO_CLASS(WIScrollContainer,WIScrollContainer);
WIScrollContainer::WIScrollContainer()
	: WIBase()
{
	SetScrollInputEnabled(true);
}

WIScrollContainer::~WIScrollContainer()
{
	if(m_hChildAdded.IsValid())
		m_hChildAdded.Remove();
	if(m_hChildRemoved.IsValid())
		m_hChildRemoved.Remove();
	std::unordered_map<WIBase*,std::vector<CallbackHandle>>::iterator it;
	for(it=m_childCallbackHandles.begin();it!=m_childCallbackHandles.end();it++)
	{
		std::vector<CallbackHandle> &handles = it->second;
		std::vector<CallbackHandle>::iterator itCb;
		for(itCb=handles.begin();itCb!=handles.end();itCb++)
		{
			CallbackHandle &hCallback = *itCb;
			if(hCallback.IsValid())
				hCallback.Remove();
		}
	}
}

void WIScrollContainer::SizeToContents()
{
	if(m_hWrapper.IsValid())
		m_hWrapper->SizeToContents();
	WIBase::SizeToContents();
}

int WIScrollContainer::GetContentWidth()
{
	auto w = GetWidth();
	if(m_hScrollBarV.IsValid() && m_hScrollBarV->IsVisible())
		w -= m_hScrollBarV->GetWidth();
	return w;
}

int WIScrollContainer::GetContentHeight()
{
	auto h = GetHeight();
	if(m_hScrollBarH.IsValid() && m_hScrollBarH->IsVisible())
		h -= m_hScrollBarH->GetHeight();
	return h;
}

Vector2i WIScrollContainer::GetContentSize()
{
	if(!m_hWrapper.IsValid())
		return Vector2i(0,0);
	return Vector2i(GetContentWidth(),GetContentHeight());
}

int WIScrollContainer::GetScrollBarWidthV()
{
	if(!m_hScrollBarV.IsValid() || !m_hScrollBarV->IsVisible())
		return 0;
	return m_hScrollBarV->GetWidth();
}
int WIScrollContainer::GetScrollBarHeightH()
{
	if(!m_hScrollBarH.IsValid() || !m_hScrollBarH->IsVisible())
		return 0;
	return m_hScrollBarH->GetHeight();
}

void WIScrollContainer::OnHScrollOffsetChanged(unsigned int offset)
{
	if(!m_hWrapper.IsValid() || offset == CUInt32(m_lastOffset.x))
		return;
	WIBase *pWrapper = m_hWrapper.get();
	pWrapper->SetX(pWrapper->GetX() -(offset -m_lastOffset.x));
	m_lastOffset.x = offset;
}
void WIScrollContainer::OnVScrollOffsetChanged(unsigned int offset)
{
	if(!m_hWrapper.IsValid() || offset == CUInt32(m_lastOffset.y))
		return;
	WIBase *pWrapper = m_hWrapper.get();
	pWrapper->SetY(pWrapper->GetY() -(offset -m_lastOffset.y));
	m_lastOffset.y = offset;
}

void WIScrollContainer::Initialize()
{
	WIBase::Initialize();
	m_hScrollBarH = CreateChild<WIScrollBar>();
	WIScrollBar *pScrollBarH = m_hScrollBarH.get<WIScrollBar>();
	pScrollBarH->SetVisible(false);
	pScrollBarH->SetZPos(1000);
	pScrollBarH->AddStyleClass("scrollbar_horizontal");
	pScrollBarH->AddCallback("OnScrollOffsetChanged",FunctionCallback<void,unsigned int>::Create(
		std::bind(&WIScrollContainer::OnHScrollOffsetChanged,this,std::placeholders::_1)
	));

	m_hScrollBarV = CreateChild<WIScrollBar>();
	WIScrollBar *pScrollBarV = m_hScrollBarV.get<WIScrollBar>();
	pScrollBarV->SetVisible(false);
	pScrollBarV->SetZPos(1000);
	pScrollBarH->AddStyleClass("scrollbar_vertical");
	pScrollBarV->AddCallback("OnScrollOffsetChanged",FunctionCallback<void,unsigned int>::Create(
		std::bind(&WIScrollContainer::OnVScrollOffsetChanged,this,std::placeholders::_1)
	));

	m_hWrapper = CreateChild<WIBase>();
	WIBase *pWrapper = m_hWrapper.get();
	m_hChildAdded = pWrapper->AddCallback("OnChildAdded",FunctionCallback<void,WIBase*>::Create(
		std::bind(&WIScrollContainer::OnWrapperChildAdded,this,std::placeholders::_1)
	));
	m_hChildRemoved = pWrapper->AddCallback("OnChildRemoved",FunctionCallback<void,WIBase*>::Create(
		std::bind(&WIScrollContainer::OnWrapperChildRemoved,this,std::placeholders::_1)
	));
}
void WIScrollContainer::ScrollCallback(Vector2 offset)
{
	WIBase::ScrollCallback(offset);
	if(m_hScrollBarH.IsValid())
		m_hScrollBarH.get<WIScrollBar>()->ScrollCallback(Vector2(offset.x,0.f));
	if(m_hScrollBarV.IsValid())
		m_hScrollBarV.get<WIScrollBar>()->ScrollCallback(Vector2(0.f,offset.y));
}
WIScrollBar *WIScrollContainer::GetHorizontalScrollBar() {return static_cast<WIScrollBar*>(m_hScrollBarH.get());}
WIScrollBar *WIScrollContainer::GetVerticalScrollBar() {return static_cast<WIScrollBar*>(m_hScrollBarV.get());}
void WIScrollContainer::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
	ScheduleUpdate();
}
void WIScrollContainer::OnChildReleased(WIBase *child)
{
	WIScrollContainer *parent = dynamic_cast<WIScrollContainer*>(child->GetParent());
	if(parent == nullptr)
		return;
	parent->OnChildRemoved(child);
}
void WIScrollContainer::OnChildSetSize(WIBase *child)
{
	WIBase *parentWrapper = child->GetParent();
	if(parentWrapper == nullptr)
		return;
	WIScrollContainer *parent = dynamic_cast<WIScrollContainer*>(parentWrapper->GetParent());
	if(parent == nullptr)
		return;
	parent->ScheduleUpdate();
}
void WIScrollContainer::OnWrapperChildAdded(WIBase *child)
{
	std::unordered_map<WIBase*,std::vector<CallbackHandle>>::iterator it = m_childCallbackHandles.insert(std::unordered_map<WIBase*,std::vector<CallbackHandle>>::value_type(child,std::vector<CallbackHandle>())).first;
	CallbackHandle hCallbackOnRemove = child->AddCallback("OnRemove",FunctionCallback<>::Create(
		std::bind(&WIScrollContainer::OnChildReleased,child)
	));
	it->second.push_back(hCallbackOnRemove);
	CallbackHandle hCallbackSetSize = child->AddCallback("SetSize",FunctionCallback<>::Create(
		std::bind(&WIScrollContainer::OnChildSetSize,child)
	));
	it->second.push_back(hCallbackSetSize);
	Update();
}
void WIScrollContainer::OnWrapperChildRemoved(WIBase *child)
{
	std::unordered_map<WIBase*,std::vector<CallbackHandle>>::iterator it = m_childCallbackHandles.find(child);
	if(it != m_childCallbackHandles.end())
	{
		std::vector<CallbackHandle> &callbacks = it->second;
		std::vector<CallbackHandle>::iterator itCb;
		for(itCb=callbacks.begin();itCb!=callbacks.end();itCb++)
		{
			CallbackHandle &hCallback = *itCb;
			if(hCallback.IsValid())
				hCallback.Remove();
		}
		m_childCallbackHandles.erase(it);
	}
	Update();
}
void WIScrollContainer::OnChildAdded(WIBase *child)
{
	WIBase::OnChildAdded(child);
	if(m_hWrapper.IsValid())
		child->SetParent(m_hWrapper.get());
}
void WIScrollContainer::Think()
{
	WIBase::Think();

}
void WIScrollContainer::OnChildRemoved(WIBase *child)
{
	WIBase::OnChildRemoved(child);
}
void WIScrollContainer::Update()
{
	WIBase::Update();
	if(!m_hWrapper.IsValid())
		return;
	WIBase *pWrapper = m_hWrapper.get();
	int w = 0;
	int h = 0;
	std::vector<WIHandle> *children = pWrapper->GetChildren();
	std::vector<WIHandle>::iterator it;
	for(it=children->begin();it!=children->end();it++)
	{
		WIHandle &hChild = *it;
		if(hChild.IsValid())
		{
			const Vector2i &posChild = hChild->GetPos();
			const Vector2i &sizeChild = hChild->GetSize();
			if(posChild.x +sizeChild.x > w)
				w = posChild.x +sizeChild.x;
			if(posChild.y +sizeChild.y > h)
				h = posChild.y +sizeChild.y;
		}
	}
	Vector2i size = GetSize();
	pWrapper->SetSize(w,h);
	if(m_hScrollBarH.IsValid())
	{
		WIScrollBar *pScrollBar = m_hScrollBarH.get<WIScrollBar>();
		if(w <= size.x)
			pScrollBar->SetVisible(false); // set scroll offset to 0
		else
		{
			pScrollBar->SetVisible(true);
			pScrollBar->SetUp(size.x,w);
			pScrollBar->SetScrollAmount(10);
			pScrollBar->SetSize(GetWidth(),10);
			pScrollBar->SetY(GetHeight() -pScrollBar->GetHeight());
		}
	}
	if(m_hScrollBarV.IsValid())
	{
		WIScrollBar *pScrollBar = m_hScrollBarV.get<WIScrollBar>();
		if(h <= size.y)
			pScrollBar->SetVisible(false);
		else
		{
			pScrollBar->SetVisible(true);
			pScrollBar->SetUp(size.y,h);
			pScrollBar->SetScrollAmount(10);
			pScrollBar->SetSize(10,GetHeight());
			pScrollBar->SetX(GetWidth() -pScrollBar->GetWidth());
		}
	}
}