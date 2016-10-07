// The MIT License (MIT)

// Copyright (c) 2016, Microsoft

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#include "BitFunnel/Plan/AbstractRow.h" // Embeds AbstractRow.
#include "BitFunnel/Plan/RowPlan.h"     // Inherits from RowPlanBase.


namespace BitFunnel
{
    class IAllocator;
    class IObjectParser;

    class RowMatchNode : public RowPlanBase
    {
    public:
        // Nodes
        class And;
        class Not;
        class Or;
        class Report;
        class Row;

        // Node builder.
        class Builder;

        //Static parsing methods.
        static RowMatchNode const & Parse(IObjectParser& parser);
        static RowMatchNode const * ParseNullable(IObjectParser& parser);
    };


    class RowMatchNode::And : public RowMatchNode
    {
    public:
        And(RowMatchNode const & left, RowMatchNode const & right);

        void Format(IObjectFormatter& formatter) const;

        NodeType GetType() const;

        RowMatchNode const & GetLeft() const;
        RowMatchNode const & GetRight() const;

        static And const & Parse(IObjectParser& parser);

    private:
        // WARNING: The persistence format depends on the order in which the
        // following two members are declared. If the order is changed, it is
        // neccesary to update the corresponding code in the constructor and
        // and the Format() method.
        RowMatchNode const & m_left;
        RowMatchNode const & m_right;

        static char const * c_childrenFieldName;
    };


    class RowMatchNode::Not : public RowMatchNode
    {
    public:
        Not(RowMatchNode const & child);
        Not(IObjectParser& parser);

        void Format(IObjectFormatter& formatter) const;

        NodeType GetType() const;

        RowMatchNode const & GetChild() const;

    private:
        RowMatchNode const & m_child;

        static char const * c_childFieldName;
    };


    class RowMatchNode::Or : public RowMatchNode
    {
    public:
        Or(RowMatchNode const & left, RowMatchNode const & right);

        void Format(IObjectFormatter& formatter) const;

        NodeType GetType() const;

        RowMatchNode const & GetLeft() const;
        RowMatchNode const & GetRight() const;

        static Or const & Parse(IObjectParser& parser);

    private:
        // WARNING: The persistence format depends on the order in which the
        // following two members are declared. If the order is changed, it is
        // neccesary to update the corresponding code in the constructor and
        // and the Format() method.
        RowMatchNode const & m_left;
        RowMatchNode const & m_right;

        static char const * c_childrenFieldName;
    };


    class RowMatchNode::Report : public RowMatchNode
    {
    public:
        Report(RowMatchNode const * child);
        Report(IObjectParser& parser);

        void Format(IObjectFormatter& formatter) const;

        NodeType GetType() const;

        RowMatchNode const * GetChild() const;

    private:
        RowMatchNode const * m_child;

        static char const * c_childFieldName;
    };


    class RowMatchNode::Row : public RowMatchNode
    {
    public:
        Row(AbstractRow const & row);
        Row(IObjectParser& parser);

        void Format(IObjectFormatter& formatter) const;

        NodeType GetType() const;

        AbstractRow const & GetRow() const;

    private:
        const AbstractRow m_row;

        static char const * c_rowFieldName;
    };


    class RowMatchNode::Builder : NonCopyable
    {
    public:
        Builder(RowMatchNode const & parent,
                IAllocator& allocator);

        Builder(RowMatchNode::NodeType nodeType,
                IAllocator& allocator);

        void AddChild(RowMatchNode const * child);

        RowMatchNode const * Complete();

        static RowMatchNode const *
        CreateReportNode(RowMatchNode const * child,
                         IAllocator& allocator);

        static RowMatchNode const *
        CreateRowNode(AbstractRow const & row,
                      IAllocator& allocator);

    private:
        IAllocator& m_allocator;
        RowMatchNode::NodeType m_targetType;
        RowMatchNode const * m_firstChild;
        RowMatchNode const * m_node;
    };
}
