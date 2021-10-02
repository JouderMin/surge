/*
** Surge Synthesizer is Free and Open Source Software
**
** Surge is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Surge was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Surge
** open source in September 2018.
*/

#include "TypeAheadTextEditor.h"
#include "MainFrame.h"

namespace Surge
{
namespace Widgets
{

void TypeAheadDataProvider::paintDataItem(int searchIndex, juce::Graphics &g, int width, int height,
                                          bool rowIsSelected)
{
    if (rowIsSelected)
    {
        g.fillAll(juce::Colours::wheat);
        g.setColour(juce::Colours::darkblue);
    }
    else
    {
        g.fillAll(juce::Colours::white);
        g.setColour(juce::Colours::black);
    }
    g.drawText(textBoxValueForIndex(searchIndex), 0, 0, width, height,
               juce::Justification::centredLeft);
}

struct TypeAheadListBoxModel : public juce::ListBoxModel
{
    TypeAheadDataProvider *provider;
    std::vector<int> search;
    TypeAhead *ta{nullptr};
    TypeAheadListBoxModel(TypeAhead *t, TypeAheadDataProvider *p) : ta(t), provider(p) {}

    void setSearch(const std::string &t) { search = provider->searchFor(t); }
    int getNumRows() override { return search.size(); }

    void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                          bool rowIsSelected) override
    {
        provider->paintDataItem(search[rowNumber], g, width, height, rowIsSelected);
    }

    void returnKeyPressed(int lastRowSelected) override
    {
        ta->dismissWithValue(lastRowSelected, provider->textBoxValueForIndex(lastRowSelected));
    }
    void escapeKeyPressed()
    {
        std::cout << "B" << std::endl;
        ta->dismissWithoutValue();
    }
};

struct TypeAheadListBox : public juce::ListBox
{
    TypeAheadListBox(const juce::String &s, juce::ListBoxModel *m) : juce::ListBox(s, m)
    {
        setColour(outlineColourId, juce::Colours::black);
        setOutlineThickness(1);
        setColour(backgroundColourId, juce::Colours::white);
    }

    bool keyPressed(const juce::KeyPress &press) override
    {
        if (press.isKeyCode(juce::KeyPress::escapeKey))
        {
            if (auto m = dynamic_cast<TypeAheadListBoxModel *>(getModel()))
            {
                std::cout << "A" << std::endl;
                m->escapeKeyPressed();
                return true;
            }
        }
        return ListBox::keyPressed(press);
    }
};

TypeAhead::TypeAhead(const std::string &l, TypeAheadDataProvider *p)
    : juce::TextEditor(l), lboxmodel(std::make_unique<TypeAheadListBoxModel>(this, p))
{
    addListener(this);
    lbox = std::make_unique<TypeAheadListBox>("TypeAhead", lboxmodel.get());
    lbox->setMultipleSelectionEnabled(false);
    lbox->setVisible(false);
}

TypeAhead::~TypeAhead() = default;

void TypeAhead::dismissWithValue(int providerIdx, const std::string &s)
{
    setText(s, juce::NotificationType::dontSendNotification);
    lbox->setVisible(false);
    grabKeyboardFocus();
    for (auto l : taList)
        l->itemSelected(providerIdx);
}

void TypeAhead::dismissWithoutValue()
{
    std::cout << "C" << std::endl;
    lbox->setVisible(false);
    grabKeyboardFocus();
    for (auto l : taList)
        l->typeaheadCanceled();
}

void TypeAhead::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{
    lbox->setVisible(false);
    for (auto l : taList)
        l->typeaheadCanceled();
}

void TypeAhead::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    lbox->setVisible(false);

    if (setToElementZeroOnReturn && lboxmodel->getNumRows() > 1)
    {
        auto r = lboxmodel->provider->textBoxValueForIndex(0);
        setText(r, juce::NotificationType::dontSendNotification);
        for (auto l : taList)
        {
            l->itemSelected(0);
        }
    }
}

void TypeAhead::parentHierarchyChanged()
{
    TextEditor::parentHierarchyChanged();
    auto p = getParentComponent();
    while (p && !dynamic_cast<Surge::Widgets::MainFrame *>(p))
    {
        p = p->getParentComponent();
    }
    if (p)
        p->addChildComponent(*lbox);
}

void TypeAhead::showLbox()
{
    auto p = getParentComponent();
    while (p && !dynamic_cast<Surge::Widgets::MainFrame *>(p))
    {
        p = p->getParentComponent();
    }

    auto b = getLocalBounds().translated(0, getHeight()).withHeight(140);
    if (p)
    {
        b = p->getLocalArea(this, b);
    }
    lbox->setBounds(b);
    lbox->setVisible(true);
    lbox->toFront(false);
}

void TypeAhead::textEditorTextChanged(TextEditor &editor)
{
    lboxmodel->setSearch(editor.getText().toStdString());

    if (!lbox->isVisible())
    {
        showLbox();
    }
    lbox->updateContent();
    lbox->repaint();
}

bool TypeAhead::keyPressed(const juce::KeyPress &press)
{
    if (press.isKeyCode(juce::KeyPress::downKey))
    {
        if (!lbox->isVisible())
        {
            lboxmodel->setSearch("");
            showLbox();

            lbox->updateContent();
            lbox->repaint();
        }
        juce::SparseSet<int> r;
        r.addRange({0, 1});
        lbox->setSelectedRows(r);
        lbox->grabKeyboardFocus();
        return true;
    }
    return TextEditor::keyPressed(press);
}
} // namespace Widgets
} // namespace Surge