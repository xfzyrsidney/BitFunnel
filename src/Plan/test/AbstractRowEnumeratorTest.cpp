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

#include "gtest/gtest.h"

#include "Allocator.h"
#include "AbstractRowEnumerator.h"
// #include "BitFunnel/Factories.h"
#include "BitFunnel/IFactSet.h"
#include "BitFunnel/ITermTable.h"
#include "BitFunnel/TermInfo.h"
// #include "MockIndexConfiguration.h"
// n#include "MockTermTable.h"
// #include "MockTermTableCollection.h"
#include "PlanRows.h"
#include "Random.h"

namespace BitFunnel
{
    namespace AbstractRowEnumeratorUnitTest
    {
        class RestrictedCapacityPlanRows : public PlanRows
        {
        public:

            RestrictedCapacityPlanRows(const IIndexConfiguration& index);

            virtual ~RestrictedCapacityPlanRows();

            const static unsigned c_maxRowsPerQuery = 5;

        protected:

            virtual unsigned GetRowCountLimit() const;
        };


        RestrictedCapacityPlanRows::RestrictedCapacityPlanRows(const IIndexConfiguration& index)
            : PlanRows(index)
        {
        }


        RestrictedCapacityPlanRows::~RestrictedCapacityPlanRows()
        {
        }


        unsigned RestrictedCapacityPlanRows::GetRowCountLimit() const
        {
            return c_maxRowsPerQuery;
        }


        static std::vector<DocIndex> CreateDefaultShardCapacities()
        {
            const std::vector<DocIndex> capacities = { 4096 };
            return capacities;
        }

        static std::vector<DocIndex> s_defaultShardCapacities(CreateDefaultShardCapacities());


        // Generate a random term list.
        void GenerateTermList(std::vector<Term>& termList, size_t count)
        {
            // Random term hash generator.
            RandomInt<Term::Hash> randomTermHashGenerator(7633134, 1, 4294967296);

            termList.clear();

            for (size_t i = 0; i < count; ++i)
            {
                const Term::Hash hash = randomTermHashGenerator();

                // hash, stream, idf.
                Term term(hash, 0, 10);

                termList.push_back(term);
            }
        }


        // Generate PlanRows for a list of terms.
        void GeneratePlanRows(IPlanRows& planRows, const std::vector<Term>& termList)
        {
            for (size_t i = 0; i < termList.size(); ++i)
            {
                AbstractRowEnumerator rowEnumerator(termList[i], planRows);
            }
        }


        // Verify planRows whose rows are truncated during the generation by AbstractRowEnumerator.
        void VerifyTruncatedPlanRows(size_t termCount)
        {
            // Test setup.
            MockIndexConfiguration index(s_defaultShardCapacities);
            PrivateHeapAllocator allocator;

            std::vector<Term> termList;

            GenerateTermList(termList, termCount);

            // First, generate a non-truncated plan row.
            PlanRows planRows(index);
            GeneratePlanRows(planRows, termList);

            // Then, generate a truncated plan row using the same
            // list of terms.
            RestrictedCapacityPlanRows truncatedPlanRows(index);
            GeneratePlanRows(truncatedPlanRows, termList);

            // Test the assumption of the test.
            TestEqual(static_cast<unsigned>(5),
                      RestrictedCapacityPlanRows::c_maxRowsPerQuery);

            // Compare the truncated plan row with the normal plan row.
            // Testing two conditions:
            // 1. The truncated plan row should have exactly RestrictedCapacityPlanRows::c_maxRowsPerQuery
            //    number of rows.
            // 2. The rows in the truncated plan rows should be in the normal plan rows as well.
            TestEqual(RestrictedCapacityPlanRows::c_maxRowsPerQuery,
                      truncatedPlanRows.GetRowCount());

            for (size_t rowIter = 0;
                 rowIter < RestrictedCapacityPlanRows::c_maxRowsPerQuery;
                 ++rowIter)
            {
                TestEqual(truncatedPlanRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetShard(),
                          planRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetShard());

                TestEqual(truncatedPlanRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetTier(),
                          planRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetTier());

                TestEqual(truncatedPlanRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetRank(),
                          planRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetRank());

                TestEqual(truncatedPlanRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetIndex(),
                          planRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetIndex());
            }
        }


        // Verify planRows generated by AbstractRowEnumerator.
        void VerifyPlanRows(size_t termCount)
        {
            MockIndexConfiguration index(s_defaultShardCapacities);
            PrivateHeapAllocator allocator;
            PlanRows planRows(index);

            std::vector<Term> termList;

            GenerateTermList(termList, termCount);

            GeneratePlanRows(planRows, termList);

            size_t rowIter = 0;

            for (size_t i = 0; i < termList.size(); ++i)
            {
                TermInfo rows(termList[i], planRows.GetTermTable(0));
                while (rows.MoveNext())
                {
                    RowId rowId = rows.Current();

                    // Verify that the rowIds are correctly set in the planRows.
                    TestEqual(rowId.GetShard(),
                              planRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetShard());

                    TestEqual(rowId.GetTier(),
                              planRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetTier());

                    TestEqual(rowId.GetRank(),
                              planRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetRank());

                    TestEqual(rowId.GetIndex(),
                              planRows.PhysicalRow(0, static_cast<unsigned>(rowIter)).GetIndex());

                    rowIter++;
                }
            }
        }


        // Verify planRows which contains special rows during the generation by AbstractRowEnumerator.
        void VerifyPlanRowsWithSpecialRows(std::vector<Term> const & termList,
                                           RowId expectedSpecialRowId,
                                           unsigned expectedSpecialRowCount,
                                           ShardId shard,
                                           IIndexConfiguration& index)
        {
            // Generate plan rows.
            PlanRows planRows(index);
            GeneratePlanRows(planRows, termList);

            // Verify the generated plan rows has the expected number of special rows.
            unsigned actualSpecialRowCount = 0;
            for (size_t rowIter = 0;
                 rowIter < planRows.GetRowCount();
                 ++rowIter)
            {
                if (planRows.PhysicalRow(shard, static_cast<unsigned>(rowIter)) == expectedSpecialRowId)
                {
                    actualSpecialRowCount++;
                }
            }

            TestEqual(actualSpecialRowCount, expectedSpecialRowCount);
        }


        TestCase(SimpleCaseOneTerm)
        {
            VerifyPlanRows(1);
        }


        TestCase(MultipleTermsCase)
        {
            VerifyPlanRows(5);
        }


        TestCase(TruncatedPlanRows)
        {
            VerifyTruncatedPlanRows(RestrictedCapacityPlanRows::c_maxRowsPerQuery);
        }


        TestCase(MatchAllRowsInPlanRows)
        {
            // For shard 0, create a term table which assigns 2 rows for all terms in all ranks.
            const std::shared_ptr<MockTermTable> termTableShard0(new MockTermTable(0, 2, 2, 2));

            // For shard 1, create a term table which assigns 1 row for rank 0 and no rows for rank
            // 3 and 6.
            const std::shared_ptr<MockTermTable> termTableShard1(new MockTermTable(1, 1, 0, 0));

            // Set these two customized term table in the index configuration.
            const std::vector<DocIndex> c_shardCapacity(2, 1);
            std::unique_ptr<IFactSet> factSet(Factories::CreateFactSet());
            std::shared_ptr<const ITermTableCollection> termTableCollection(new MockTermTableCollection(c_shardCapacity, *factSet));
            static_cast<MockTermTableCollection*>(const_cast<ITermTableCollection*>(termTableCollection.get()))->SetTermTable(0, termTableShard0);
            static_cast<MockTermTableCollection*>(const_cast<ITermTableCollection*>(termTableCollection.get()))->SetTermTable(1, termTableShard1);
            MockIndexConfiguration index(termTableCollection);

            // Get the row Id of the match all rows from term table for shard 1 (Shard 1 will get the
            // match-all padding rows).
            TermInfo matchAllTerm(ITermTable::GetMatchAllTerm(), *(index.GetTermTables().GetTermTable(1)));
            LogAssertB(matchAllTerm.MoveNext());
            const RowId matchAllRowId = matchAllTerm.Current();

            const unsigned termCount = 1;
            std::vector<Term> termList;
            GenerateTermList(termList, termCount);

            // Since for shard 1, the term has no rows for rank 3 and 6, so there will be 2 match-all
            // padding rows for rank 3 and 6. In total, there will be 4 match-all rows.
            const unsigned expectedMatchAllRowCount = 4;

            // Verify there should be 4 match-all padding rows in shard 1.
            VerifyPlanRowsWithSpecialRows(termList, matchAllRowId, expectedMatchAllRowCount, 1, index);

            // Verify there should be no match-all padding rows in shard 0.
            VerifyPlanRowsWithSpecialRows(termList, matchAllRowId, 0, 0, index);
        }


        TestCase(MatchNoneRowsInPlanRows)
        {
            // Create a term which in the HDD tier to trigger the
            // generation of match none rows.
            Term termInHDDTier(1234, Stream::Full, 10, HDDTier);

            const std::vector<DocIndex> c_shardCapacity(1, 1);
            MockIndexConfiguration index(c_shardCapacity);

            // Get the row Id of the match none rows from term table.
            TermInfo matchNoneTerm(ITermTable::GetMatchNoneTerm(), *(index.GetTermTables().GetTermTable(0)));
            LogAssertB(matchNoneTerm.MoveNext());
            const RowId matchNoneRowId = matchNoneTerm.Current();

            // Get the expected number of rows for the special term in the term table.
            unsigned expectedMatchNoneRowCount = 0;
            ITermTableCollection const & termTableCollection = index.GetTermTables();
            std::shared_ptr<ITermTable const> termTable = termTableCollection.GetTermTable(0);
            TermInfo matchNoneTermInfo(termInHDDTier, *termTable);
            while (matchNoneTermInfo.MoveNext())
            {
                ++expectedMatchNoneRowCount;
            }

            const unsigned termCount = 4;
            std::vector<Term> termList;
            GenerateTermList(termList, termCount);

            termList.push_back(termInHDDTier);

            VerifyPlanRowsWithSpecialRows(termList, matchNoneRowId, expectedMatchNoneRowCount, 0, index);
        }
    }
}
