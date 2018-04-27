#include <SAMRAI/geom/CartesianPatchGeometry.h>
#include <SAMRAI/pdat/CellData.h>
#include <SAMRAI/pdat/CellDataFactory.h>
#include <SAMRAI/pdat/NodeDataFactory.h>
#include <SAMRAI/pdat/NodeGeometry.h>
#include <SAMRAI/tbox/SAMRAIManager.h>
#include <SAMRAI/tbox/SAMRAI_MPI.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "data/field/field.h"
#include "data/field/field_data.h"
#include "data/field/field_variable.h"
#include "data/grid/gridlayout.h"
#include "data/ndarray/ndarray_vector.h"
#include "field_data_test_param.h"
#include "utilities/point/point.h"


using testing::Eq;
namespace PHARE
{
using Field1D = Field<NdArrayVector1D<>, HybridQuantity::Scalar>;

using FieldDataTest1DOrder1 = FieldDataTestParam<Layout::Yee, 1, 1, Field1D>;
using FieldDataTest1DOrder2 = FieldDataTestParam<Layout::Yee, 1, 2, Field1D>;
using FieldDataTest1DOrder3 = FieldDataTestParam<Layout::Yee, 1, 3, Field1D>;

using FieldDataTestList
    = ::testing::Types<FieldDataTest1DOrder1, FieldDataTest1DOrder2, FieldDataTest1DOrder3>;


TYPED_TEST_CASE_P(AFieldData1DCenteredOnEx);


TYPED_TEST_P(AFieldData1DCenteredOnEx, CopyWithOverlapLikeACellData)
{
    std::shared_ptr<SAMRAI::hier::BoxGeometry> cell0Geom
        = std::make_shared<SAMRAI::pdat::CellGeometry>(this->param.patch0.getBox(), this->ghosts);

    std::shared_ptr<SAMRAI::hier::BoxGeometry> cell1Geom
        = std::make_shared<SAMRAI::pdat::CellGeometry>(this->param.patch1.getBox(), this->ghosts);

    int lower = 6;
    int upper = 9;

    auto const& layout0 = this->param.field0Data->gridLayout;

    if (layout0.order() == 2)
    {
        upper = 15;
    }
    else if (layout0.order() == 3)
    {
        upper = 20;
    }

    SAMRAI::hier::Box srcMask{SAMRAI::hier::Index{this->dim, lower},
                              SAMRAI::hier::Index{this->dim, upper}, this->blockId};
    SAMRAI::hier::Box fillBox{SAMRAI::hier::Index{this->dim, lower},
                              SAMRAI::hier::Index{this->dim, upper}, this->blockId};

    bool overwriteInterior{true};

    SAMRAI::hier::Transformation transformation{SAMRAI::hier::IntVector::getZero(this->dim)};


    auto fieldOverlap
        = std::dynamic_pointer_cast<FieldOverlap<1>>(this->param.field0Geom->calculateOverlap(
            *this->param.field1Geom, srcMask, fillBox, overwriteInterior, transformation));

    auto cellOverlap
        = std::dynamic_pointer_cast<SAMRAI::pdat::CellOverlap>(cell0Geom->calculateOverlap(
            *cell1Geom, srcMask, fillBox, overwriteInterior, transformation));


    this->cell0Data->fillAll(0.0);
    this->cell1Data->fillAll(1.0);

    auto& field0 = this->param.field0Data->field;


    this->param.field0Data->copy(*this->param.field1Data, *fieldOverlap);

    this->cell0Data->copy(*this->cell1Data, *cellOverlap);

    auto iStart = this->param.field0Data->gridLayout.ghostStartIndex(field0, Direction::X);
    auto iEnd   = this->param.field0Data->gridLayout.ghostEndIndex(field0, Direction::X);


    double const* cellDataStart = this->cell0Data->getPointer();
    for (auto ix = iStart; ix <= iEnd; ++ix)
    {
        EXPECT_THAT(field0(ix), Eq(cellDataStart[ix]));
    }
}
TYPED_TEST_P(AFieldData1DCenteredOnEx, CopyWithPeriodicsLikeACellData)
{
    std::shared_ptr<SAMRAI::hier::BoxGeometry> cell0Geom
        = std::make_shared<SAMRAI::pdat::CellGeometry>(this->param.patch0.getBox(), this->ghosts);

    std::shared_ptr<SAMRAI::hier::BoxGeometry> cell1Geom
        = std::make_shared<SAMRAI::pdat::CellGeometry>(this->param.patch1.getBox(), this->ghosts);


    bool overwriteInterior{true};

    SAMRAI::hier::IntVector shift{this->param.patch0.getBox().lower()
                                  - this->param.patch1.getBox().upper()};
    SAMRAI::hier::Transformation transformation{shift};

    SAMRAI::hier::Box srcMask{this->cell1Data->getBox()};
    SAMRAI::hier::Box fillMask{this->cell0Data->getGhostBox()};


    auto fieldOverlap
        = std::dynamic_pointer_cast<FieldOverlap<1>>(this->param.field0Geom->calculateOverlap(
            *this->param.field1Geom, srcMask, fillMask, overwriteInterior, transformation));

    auto cellOverlap
        = std::dynamic_pointer_cast<SAMRAI::pdat::CellOverlap>(cell0Geom->calculateOverlap(
            *cell1Geom, srcMask, fillMask, overwriteInterior, transformation));


    this->cell0Data->fillAll(0.0);
    this->cell1Data->fillAll(1.0);

    auto& field0 = this->param.field0Data->field;


    this->param.field0Data->copy(*this->param.field1Data, *fieldOverlap);

    this->cell0Data->copy(*this->cell1Data, *cellOverlap);

    auto iStart = this->param.field0Data->gridLayout.ghostStartIndex(field0, Direction::X);
    auto iEnd   = this->param.field0Data->gridLayout.ghostEndIndex(field0, Direction::X);


    double const* cellDataStart = this->cell0Data->getPointer();
    for (auto ix = iStart; ix <= iEnd; ++ix)
    {
        EXPECT_THAT(field0(ix), Eq(cellDataStart[ix]));
    }
}


REGISTER_TYPED_TEST_CASE_P(AFieldData1DCenteredOnEx, CopyWithOverlapLikeACellData,
                           CopyWithPeriodicsLikeACellData);


INSTANTIATE_TYPED_TEST_CASE_P(TestWithOrderFrom1To3That, AFieldData1DCenteredOnEx,
                              FieldDataTestList);


TYPED_TEST_CASE_P(AFieldData1DCenteredOnEy);

TYPED_TEST_P(AFieldData1DCenteredOnEy, CopyWithOverlapLikeANodeData)
{
    auto& patch0 = this->patch1d.patch0;
    auto& patch1 = this->patch1d.patch1;


    std::shared_ptr<SAMRAI::hier::BoxGeometry> node0Geom
        = std::make_shared<SAMRAI::pdat::NodeGeometry>(patch0.getBox(), this->ghosts);

    std::shared_ptr<SAMRAI::hier::BoxGeometry> node1Geom
        = std::make_shared<SAMRAI::pdat::NodeGeometry>(patch1.getBox(), this->ghosts);

    int lower = 6;
    int upper = 9;

    auto const& layout0 = this->param.field0Data->gridLayout;

    if (layout0.order() == 2)
    {
        upper = 15;
    }
    else if (layout0.order() == 3)
    {
        upper = 20;
    }

    SAMRAI::hier::Box srcMask{SAMRAI::hier::Index{this->dim, lower},
                              SAMRAI::hier::Index{this->dim, upper}, this->blockId};
    SAMRAI::hier::Box fillBox{SAMRAI::hier::Index{this->dim, lower},
                              SAMRAI::hier::Index{this->dim, upper}, this->blockId};

    bool overwriteInterior{true};



    SAMRAI::hier::Transformation transformation{SAMRAI::hier::IntVector::getZero(this->dim)};

    auto fieldOverlap
        = std::dynamic_pointer_cast<FieldOverlap<1>>(this->param.field0Geom->calculateOverlap(
            *this->param.field1Geom, srcMask, fillBox, overwriteInterior, transformation));

    auto nodeOverlap
        = std::dynamic_pointer_cast<SAMRAI::pdat::NodeOverlap>(node0Geom->calculateOverlap(
            *node1Geom, srcMask, fillBox, overwriteInterior, transformation));


    this->node0Data->fillAll(0.0);
    this->node1Data->fillAll(1.0);

    auto& field0 = this->param.field0Data->field;



    this->param.field0Data->copy(*this->param.field1Data, *fieldOverlap);

    this->node0Data->copy(*this->node1Data, *nodeOverlap);


    auto iStart = layout0.ghostStartIndex(field0, Direction::X);
    auto iEnd   = layout0.ghostEndIndex(field0, Direction::X);

    double const* nodeDataStart = this->node0Data->getPointer();

    for (auto ix = iStart; ix <= iEnd; ++ix)
    {
        EXPECT_THAT(field0(ix), Eq(nodeDataStart[ix]));
    }
}


TYPED_TEST_P(AFieldData1DCenteredOnEy, CopyWithPeriodicsLikeANodeData)
{
    auto& patch0 = this->patch1d.patch0;
    auto& patch1 = this->patch1d.patch1;


    std::shared_ptr<SAMRAI::hier::BoxGeometry> node0Geom
        = std::make_shared<SAMRAI::pdat::NodeGeometry>(patch0.getBox(), this->ghosts);

    std::shared_ptr<SAMRAI::hier::BoxGeometry> node1Geom
        = std::make_shared<SAMRAI::pdat::NodeGeometry>(patch1.getBox(), this->ghosts);


    auto const& layout0 = this->param.field0Data->gridLayout;

    bool overwriteInterior{true};


    SAMRAI::hier::IntVector shift{this->param.patch0.getBox().lower()
                                  - this->param.patch1.getBox().upper()};
    SAMRAI::hier::Transformation transformation{shift};


    SAMRAI::hier::Box srcMask{this->node1Data->getBox()};
    SAMRAI::hier::Box fillMask{this->node0Data->getGhostBox()};


    auto fieldOverlap
        = std::dynamic_pointer_cast<FieldOverlap<1>>(this->param.field0Geom->calculateOverlap(
            *this->param.field1Geom, srcMask, fillMask, overwriteInterior, transformation));

    auto nodeOverlap
        = std::dynamic_pointer_cast<SAMRAI::pdat::NodeOverlap>(node0Geom->calculateOverlap(
            *node1Geom, srcMask, fillMask, overwriteInterior, transformation));


    this->node0Data->fillAll(0.0);
    this->node1Data->fillAll(1.0);

    auto& field0 = this->param.field0Data->field;



    this->param.field0Data->copy(*this->param.field1Data, *fieldOverlap);

    this->node0Data->copy(*this->node1Data, *nodeOverlap);


    auto iStart = layout0.ghostStartIndex(field0, Direction::X);
    auto iEnd   = layout0.ghostEndIndex(field0, Direction::X);

    double const* nodeDataStart = this->node0Data->getPointer();

    for (auto ix = iStart; ix <= iEnd; ++ix)
    {
        EXPECT_THAT(field0(ix), Eq(nodeDataStart[ix]));
    }
}


REGISTER_TYPED_TEST_CASE_P(AFieldData1DCenteredOnEy, CopyWithOverlapLikeANodeData,
                           CopyWithPeriodicsLikeANodeData);


INSTANTIATE_TYPED_TEST_CASE_P(TestWithOrderFrom1To3That, AFieldData1DCenteredOnEy,
                              FieldDataTestList);

} // namespace PHARE


int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    SAMRAI::tbox::SAMRAI_MPI::init(&argc, &argv);
    SAMRAI::tbox::SAMRAIManager::initialize();
    SAMRAI::tbox::SAMRAIManager::startup();


    int testResult = RUN_ALL_TESTS();

    // Finalize
    SAMRAI::tbox::SAMRAIManager::shutdown();
    SAMRAI::tbox::SAMRAIManager::finalize();
    SAMRAI::tbox::SAMRAI_MPI::finalize();

    return testResult;
}