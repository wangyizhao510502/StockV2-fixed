#include "pch.h"
#include "StockItem.h"
#include "StockV2.h"
#include <wx/dcgraph.h>

const wchar_t *LStockItem::GetItemName() const
{
    // index == -1 表示"未配置股票"占位项, 用于引导用户到"管理股票列表"。
    if (index < 0) {
        return L"股票插件（未配置股票）";
    }
    auto data = g_data.GetStockByIndex(index);
    if (data) {
        m_displayName = std::wstring(data->GetDisplayName().wc_str());
        return m_displayName.c_str();
    }
    return UtilResHlp.StringRes(IDS_LOADING);
}

const wchar_t *LStockItem::GetItemId() const
{
    // 使用成员变量缓存, 保证每个 LStockItem 实例有独立且稳定的 ID。
    // 旧实现使用 static 局部变量, 所有实例共享同一块内存, 在多只股票时 ID 会被覆盖,
    // 导致 TrafficMonitor 主程序中显示项冲突/错位。
    m_itemId = L"qL0KmmYi";
    m_itemId += std::to_wstring(index);
    return m_itemId.c_str();
}

const wchar_t *LStockItem::GetItemLableText() const
{
    return L"";
}

const wchar_t *LStockItem::GetItemValueText() const
{
    return L"";
}

bool LStockItem::IsCustomDraw() const
{
    return true;
}

void LStockItem::CalculateColumnWidths(CDC* pDC, int& nameWidth, int& priceWidth, int& changeWidth) const
{
    nameWidth = 0;
    priceWidth = 0;
    changeWidth = 0;

    bool includeName = g_data.IsDisplayName();
    auto allStocks = g_data.AllStocks();
    for (const auto& stock : allStocks)
    {
        if (!stock)
        {
            continue;
        }

        if (includeName)
        {
            wxString nameText = stock->GetDisplayName() + wxT(": ");
            CSize nameSize = pDC->GetTextExtent(nameText.wc_str());
            if (nameSize.cx > nameWidth)
            {
                nameWidth = nameSize.cx;
            }
        }

        wxString priceText = g_data.IsPriorityDisplayChanged() ? stock->GetChangePrice() : stock->GetCurrentPrice();
        CSize priceSize = pDC->GetTextExtent(priceText.wc_str());
        if (priceSize.cx > priceWidth)
        {
            priceWidth = priceSize.cx;
        }

        CSize changeSize = pDC->GetTextExtent(stock->changeFluctuation.wc_str());
        if (changeSize.cx > changeWidth)
        {
            changeWidth = changeSize.cx;
        }
    }
}

int LStockItem::GetItemWidthEx(void* hDC) const
{
    CDC* pDC = CDC::FromHandle((HDC)hDC);

    auto data = g_data.GetStockByIndex(index);
    if (data == nullptr)
    {
        return 0;
    }

    int nameWidth = 0, priceWidth = 0, changeWidth = 0;
    CalculateColumnWidths(pDC, nameWidth, priceWidth, changeWidth);

    const int COLUMN_SPACING = 8;
    int width = priceWidth + COLUMN_SPACING + changeWidth;
    if (g_data.IsDisplayName() && nameWidth > 0)
    {
        width += nameWidth + COLUMN_SPACING;
    }

    LLOG_DEBUG("GetItemWidthEx: name=%d price=%d change=%d total=%d", nameWidth, priceWidth, changeWidth, width);
    return width;
}

void LStockItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    auto data = g_data.GetStockByIndex(index);
    if (data == nullptr) {
        return;
    }

    // 绘图句柄
    CDC* pDC = CDC::FromHandle((HDC)hDC);

    // 矩形区域
    CRect rect(CPoint(x, y), CSize(w, h));

    // 文本颜色
    COLORREF color_default;
    COLORREF color_red;
    COLORREF color_green;
    if (dark_mode)
    {
        color_default = RGB(255, 255, 255);
        color_red = RGB(255, 121, 120);
        color_green = RGB(111, 215, 149);
    }
    else
    {
        color_default = RGB(0, 0, 0);
        color_red = RGB(195, 0, 0);
        color_green = RGB(46, 139, 87);
    }

    const int COLUMN_SPACING = 8;

    int nameWidth = 0, priceWidth = 0, changeWidth = 0;
    CalculateColumnWidths(pDC, nameWidth, priceWidth, changeWidth);

    CRect rect_remain{ rect };

    // 绘制名称：按所有股票中最大宽度右对齐，使冒号位置统一。
    if (g_data.IsDisplayName() && nameWidth > 0)
    {
        pDC->SetTextColor(color_default);
        CString displayContent = (data->GetDisplayName() + wxT(": ")).wc_str();
        CRect rect_name{ rect_remain };
        rect_name.right = rect_name.left + nameWidth;
        pDC->DrawText(displayContent, rect_name, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);

        rect_remain.left = rect_name.right + COLUMN_SPACING;
    }

    // 绘制价格（保留小数）：按最大宽度右对齐，使该列右边缘统一。
    wxString priceText = g_data.IsPriorityDisplayChanged() ? data->GetChangePrice() : data->GetCurrentPrice();

    if (g_data.IsDisplayColor())
    {
        bool isUp = data->changeFluctuation.find('+') != wxString::npos;
        if (isUp)
            pDC->SetTextColor(color_red);
        else
            pDC->SetTextColor(color_green);
    }
    else
    {
        pDC->SetTextColor(color_default);
    }

    CRect rect_price{ rect_remain };
    rect_price.right = rect_price.left + priceWidth;
    pDC->DrawText(priceText.wc_str(), rect_price, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_RIGHT);

    // 绘制涨跌幅：在价格列右侧固定间距处左对齐，使涨跌幅列左边缘统一。
    CRect rect_change{ rect_remain };
    rect_change.left = rect_price.right + COLUMN_SPACING;
    rect_change.right = rect_change.left + changeWidth;
    pDC->DrawText(data->changeFluctuation.wc_str(), rect_change, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_LEFT);
}

const wchar_t *LStockItem::GetItemValueSampleText() const
{
    return L"--";
}

int LStockItem::OnMouseEvent(MouseEventType type, int x, int y, void *hWnd, int flag)
{
    CWnd *pWnd = CWnd::FromHandle((HWND)hWnd);
    LLOG_DEBUG("OnMouseEvent: %d", type);
    CPoint ptScreen = CPoint(x, y);
    switch (type)
    {
    case IPluginItem::MT_RCLICKED:
        LStockPlugin::Instance()->ShowStockViewMenu(hWnd, ptScreen, g_data.GetStockByIndex(index));
        return 1;

    case IPluginItem::MT_LCLICKED:
    {
        LStockPlugin::Instance()->ShowStockView(index);
    }
    default:
        break;
    }
    return 0;
}
