#pragma once

#include <iosfwd>           // std::istream member.
#include <utility>          // std::pair return value.
#include <vector>           // std::vector member.

#include "BitFunnel/Term.h" // Term template parameter.


namespace BitFunnel
{
    class DocumentFrequencyTable
    {
    public:
        class Entry;

        // Constructs a DocumentFrequencyTable from data previously persisted
        // to a stream by DocumentFrequencyTableBuilder::WriteFrequencies().
        // The file format is a sequence of entries, one per line. Each entry
        // consists of the following comma-separated fields:
        //    term hash (16 digit hexidecimal)
        //    gram size (e.g. 1 for unigram, 2 for bigram phrase, etc.)
        //    stream id (e.g. 0 for body, 1 for title, etc.)
        //    frequency of term in corpus (double precision floating point)
        // Entries must be ordered by non-increasing frequency.
        DocumentFrequencyTable(std::istream& input);

        // Returns the entry corresponding a specific index.
        Entry const & operator[](size_t index) const;

        std::vector<Entry>::const_iterator begin() const;
        std::vector<Entry>::const_iterator end() const;

        size_t size() const;

        class Entry
        {
        public:
            Entry(Term term, double frequency)
                : m_term(term),
                m_frequency(frequency)
            {
            }

            Term GetTerm() const
            {
                return m_term;
            }

            double GetFrequency() const
            {
                return m_frequency;
            }

        private:
            Term m_term;
            double m_frequency;
        };

    private:
        std::vector<Entry> m_entries;
    };
}