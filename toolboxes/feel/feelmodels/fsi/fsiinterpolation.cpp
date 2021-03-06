namespace Feel
{
namespace FeelModels
{
template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::initInterpolation()
{

    if ( this->fluidModel()->doRestart() )
    {
        this->fluidModel()->meshALE()->revertReferenceMesh();
        // need to rebuild this dof point because updated in meshale after restart
        this->fluidModel()->meshALE()->displacement()->functionSpace()->rebuildDofPoints();
    }

    boost::mpi::timer btime;double thet;
    if (this->solidModel()->isStandardModel())
    {
        //---------------------------------------------//
        this->initDispInterpolation();
        std::ostringstream ostr1;ostr1<<btime.elapsed()<<"s";
        if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","initDispInterpolation","done in "+ostr1.str(),
                                                   this->worldComm(),this->verboseAllProc());
        btime.restart();
        //---------------------------------------------//
        this->initStressInterpolation();
        std::ostringstream ostr2;ostr2<<btime.elapsed()<<"s";
        if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","initStressInterpolation","done in "+ostr2.str(),
                                                   this->worldComm(),this->verboseAllProc());
        btime.restart();
        //---------------------------------------------//
        if (this->fsiCouplingType()=="Semi-Implicit" ||
            this->fsiCouplingBoundaryCondition()=="robin-neumann-generalized" ||
            this->fsiCouplingBoundaryCondition()=="robin-neumann" || this->fsiCouplingBoundaryCondition()=="robin-robin" ||
            this->fsiCouplingBoundaryCondition()=="robin-robin-genuine" || this->fsiCouplingBoundaryCondition()=="nitsche" )
        {
            this->initVelocityInterpolation();
            std::ostringstream ostr3;ostr3<<btime.elapsed()<<"s";
            if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","initVelocityInterpolation","done in "+ostr3.str(),
                                                       this->worldComm(),this->verboseAllProc());
        }
        btime.restart();
        //---------------------------------------------//
#if 0
        if (this->fsiCouplingBoundaryCondition()=="robin-neumann")
        {
            this->initStressInterpolationS2F();
            std::ostringstream ostr4;ostr4<<btime.elapsed()<<"s";
            if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","initStressInterpolationS2F","done in "+ostr4.str(),
                                                       this->worldComm(),this->verboseAllProc());

        }
#endif
        //---------------------------------------------//
        if ( /*this->fsiCouplingBoundaryCondition()=="robin-neumann" ||*/ this->fsiCouplingBoundaryCondition()=="robin-robin" ||
             this->fsiCouplingBoundaryCondition()=="robin-robin-genuine" || this->fsiCouplingBoundaryCondition()=="nitsche" )
        {
            this->initVelocityInterpolationF2S();
        }
    }
    else if ( this->solidModel()->is1dReducedModel() )
    {
        //---------------------------------------------//
        this->initDisp1dToNdInterpolation();
        thet = btime.elapsed();
        if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","initDisp1dToNdInterpolation",
                                                   (boost::format("done in %1%s")% thet).str(),
                                                   this->worldComm(),this->verboseAllProc());
        btime.restart();
        //---------------------------------------------//
        this->initStress1dToNdInterpolation();
        thet = btime.elapsed();
        if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","initStress1dToNdInterpolation",
                                                   (boost::format("done in %1%s")% thet).str(),
                                                   this->worldComm(),this->verboseAllProc());
        btime.restart();
        //---------------------------------------------//
        if (this->fsiCouplingType()=="Semi-Implicit" || this->fsiCouplingBoundaryCondition()=="robin-neumann-generalized")
        {
            this->initVelocity1dToNdInterpolation();
            thet = btime.elapsed();
            if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","initVelocity1dToNdInterpolation",
                                                       (boost::format("done in %1%s")% thet).str(),
                                                       this->worldComm(),this->verboseAllProc());
        }
        //---------------------------------------------//
    }


    if ( this->fluidModel()->doRestart() )
    {
        this->fluidModel()->meshALE()->revertMovingMesh();
        this->fluidModel()->meshALE()->displacement()->functionSpace()->rebuildDofPoints();
    }

}

//-----------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------//



template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::initDispInterpolation()
{
    if (M_interfaceFSIisConforme)
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank() )
            std::cout << "initDispInterpolation() CONFORME"  << std::endl;
        M_opDisp2dTo2dconf = opInterpolation(_domainSpace=this->solidModel()->functionSpaceDisplacement(),
                                             _imageSpace=this->fluidModel()->meshALE()->displacement()->functionSpace(),
                                             _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                             _type=InterpolationConforme(),
                                             _backend=this->fluidModel()->backend() );
    }
    else
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initDispInterpolation() NONCONFORME" << std::endl;
        M_opDisp2dTo2dnonconf = opInterpolation(_domainSpace=this->solidModel()->functionSpaceDisplacement(),
                                                _imageSpace=this->fluidModel()->meshALE()->displacement()->functionSpace(),
                                                _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                                _type=InterpolationNonConforme(),
                                                _backend=this->fluidModel()->backend() );
    }
}


//-----------------------------------------------------------------------------------//

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::initDisp1dToNdInterpolation()
{
    if ( this->fluidModel()->markersNameMovingBoundary().empty() ) return;

    if (M_interfaceFSIisConforme)
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initDisp1dToNdInterpolation() CONFORME"  << std::endl;
        M_opDisp1dToNdconf = opInterpolation(_domainSpace=this->solidModel()->fieldDisplacementVect1dReduced().functionSpace(),
                                             _imageSpace=this->fluidModel()->meshALE()->displacement()->functionSpace(),//M_fluid->meshDisplacementOnInterface().functionSpace(),
                                             _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                             _type=InterpolationConforme(),
                                             _backend=this->fluidModel()->backend() );
    }
    else
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initDisp1dToNdInterpolation() NONCONFORME" << std::endl;
        M_opDisp1dToNdnonconf = opInterpolation(_domainSpace=this->solidModel()->fieldDisplacementVect1dReduced().functionSpace(),
                                                _imageSpace=this->fluidModel()->meshALE()->displacement()->functionSpace(),//M_fluid->meshDisplacementOnInterface().functionSpace(),
                                                _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                                _type=InterpolationNonConforme(),
                                                _backend=this->fluidModel()->backend() );
    }
}

//-----------------------------------------------------------------------------------//

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::initStressInterpolation()
{
    std::vector<int> saveActivities_stress = M_fluidModel->fieldNormalStressRefMesh().functionSpace()->worldComm().activityOnWorld();
    if (M_fluidModel->worldComm().globalSize()>1 && !M_fluidModel->functionSpace()->hasEntriesForAllSpaces() )
        M_fluidModel->fieldNormalStressRefMesh().functionSpace()->worldComm().applyActivityOnlyOn(0/*VelocityWorld*/);

    if (M_interfaceFSIisConforme)
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initStressInterpolation() CONFORME" << std::endl;
        M_opStress2dTo2dconf = opInterpolation(_domainSpace=this->fluidModel()->fieldNormalStressRefMesh().functionSpace(),
                                               _imageSpace=this->solidModel()->fieldNormalStressFromFluidPtr()->functionSpace(),
                                               _range=markedfaces(this->solidModel()->mesh(),this->solidModel()->markerNameFSI()),
                                               _type=InterpolationConforme(),
                                               _backend=this->fluidModel()->backend() );
    }
    else
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initStressInterpolation() NONCONFORME" << std::endl;
        M_opStress2dTo2dnonconf = opInterpolation(_domainSpace=this->fluidModel()->fieldNormalStressRefMesh().functionSpace(),
                                                  _imageSpace=this->solidModel()->fieldNormalStressFromFluidPtr()->functionSpace(),
                                                  _range=markedfaces(this->solidModel()->mesh(),this->solidModel()->markerNameFSI()),
                                                  _type=InterpolationNonConforme(true,true,true,15),
                                                  _backend=this->fluidModel()->backend() );
    }
    // revert initial activities
    if (M_fluidModel->worldComm().globalSize()>1  && !M_fluidModel->functionSpace()->hasEntriesForAllSpaces() )
        M_fluidModel->fieldNormalStressRefMeshPtr()->functionSpace()->worldComm().setIsActive(saveActivities_stress);
}

//-----------------------------------------------------------------------------------//

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::initStress1dToNdInterpolation()
{
    if (M_interfaceFSIisConforme)
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initStress1dToNdInterpolation() CONFORME" << std::endl;
        M_opStress1dToNdconf = opInterpolation(_domainSpace=M_fluidModel->fieldNormalStressRefMesh().functionSpace(),
                                               _imageSpace=M_solidModel->fieldStressVect1dReduced().functionSpace(),
                                               _range=elements(M_solidModel->mesh1dReduced()),
                                               _type=InterpolationConforme(),
                                               _backend=M_fluidModel->backend() );

    }
    else
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initStress1dToNdInterpolation() NONCONFORME" << std::endl;
        M_opStress1dToNdnonconf = opInterpolation(_domainSpace=M_fluidModel->fieldNormalStressRefMesh().functionSpace(),
                                                  _imageSpace=M_solidModel->fieldStressVect1dReduced().functionSpace(),
                                                  _range=elements(M_solidModel->mesh1dReduced()),
                                                  _type=InterpolationNonConforme(),
                                                  _backend=M_fluidModel->backend() );
    }
}

//-----------------------------------------------------------------------------------//

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::initVelocityInterpolation()
{
    if (M_interfaceFSIisConforme)
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initVelocityInterpolation() CONFORME" << std::endl;
        M_opVelocity2dTo2dconf = opInterpolation(_domainSpace=this->solidModel()->fieldVelocity().functionSpace(),
                                                 _imageSpace=this->fluidModel()->meshVelocity2().functionSpace(),
                                                 _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                                 _type=InterpolationConforme(),
                                                 _backend=this->fluidModel()->backend() );

    }
    else
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initVelocityInterpolation() NONCONFORME" << std::endl;
        M_opVelocity2dTo2dnonconf = opInterpolation(_domainSpace=this->solidModel()->fieldVelocity().functionSpace(),
                                                    _imageSpace=this->fluidModel()->meshVelocity2().functionSpace(),
                                                    _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                                    _type=InterpolationNonConforme(),
                                                    _backend=this->fluidModel()->backend() );
    }
}

//-----------------------------------------------------------------------------------//

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::initVelocity1dToNdInterpolation()
{
    if (M_interfaceFSIisConforme)
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initVelocity1dToNdInterpolation() CONFORME" << std::endl;
        M_opVelocity1dToNdconf = opInterpolation(_domainSpace=this->solidModel()->fieldVelocityVect1dReduced().functionSpace(),
                                                 _imageSpace=this->fluidModel()->meshVelocity2().functionSpace(),
                                                 _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                                 _type=InterpolationConforme(),
                                                 _backend=this->fluidModel()->backend() );
    }
    else
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initVelocity1dToNdInterpolation() NONCONFORME" << std::endl;
        M_opVelocity1dToNdnonconf = opInterpolation(_domainSpace=this->solidModel()->fieldVelocityVect1dReduced().functionSpace(),
                                                    _imageSpace=this->fluidModel()->meshVelocity2().functionSpace(),
                                                    _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                                    _type=InterpolationNonConforme(),
                                                    _backend=this->fluidModel()->backend() );
    }
}

//-----------------------------------------------------------------------------------//

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::initVelocityInterpolationF2S()
{

    if (M_interfaceFSIisConforme)
    {
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initVelocityInterpolationF2S() CONFORME" << std::endl;
        M_opVelocity2dTo2dconfF2S = opInterpolation(_domainSpace=this->fluidModel()->functionSpaceVelocity(),
                                                    _imageSpace=this->solidModel()->fieldVelocityInterfaceFromFluidPtr()->functionSpace(),
                                                    _range=markedfaces(this->solidModel()->mesh(),this->solidModel()->markerNameFSI()),
                                                    _type=InterpolationConforme(),
                                                    _backend=this->fluidModel()->backend() );

    }
    else
    {
#if 0
        if (this->verbose() && this->fluidModel()->worldComm().isMasterRank())
            std::cout << "initVelocityInterpolation() NONCONFORME" << std::endl;
        M_opVelocity2dTo2dnonconfF2S = opInterpolation(_domainSpace=this->fluidModel()->meshVelocity2().functionSpace(),
                                                       _imageSpace=this->solidModel()->fieldVelocity()->functionSpace(),
                                                       _range=markedfaces(this->solidModel()->mesh(),this->solidModel()->markerNameFSI()),
                                                       _type=InterpolationNonConforme(),
                                                       _backend=this->fluidModel()->backend() );
#endif
    }
}

//-----------------------------------------------------------------------------------//

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::initStressInterpolationS2F()
{
    M_opStress2dTo2dconfS2F = opInterpolation(_domainSpace=this->solidModel()->fieldNormalStressFromStructPtr()->functionSpace(),
                                              _imageSpace=this->fluidModel()->normalStressFromStruct()->functionSpace(),
                                              _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                              _type=InterpolationConforme(),
                                              _backend=this->fluidModel()->backend() );
}

//-----------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------//

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::transfertDisplacement()
{
    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertDisplacement", "start",
                                               this->worldComm(),this->verboseAllProc());

    if (M_solidModel->isStandardModel())
    {
        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opDisp2dTo2dconf ) << "interpolation operator not build";
            M_opDisp2dTo2dconf->apply(M_solidModel->fieldDisplacement(),
                                      *(M_fluidModel->meshDisplacementOnInterface()) );
        }
        else
        {
            CHECK( M_opDisp2dTo2dnonconf ) << "interpolation operator not build";
            M_opDisp2dTo2dnonconf->apply(M_solidModel->fieldDisplacement(),
                                         *(M_fluidModel->meshDisplacementOnInterface()) );
        }
    }
    else if ( M_solidModel->is1dReducedModel() )
    {
        M_solidModel->updateInterfaceDispFrom1dDisp();
        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opDisp1dToNdconf ) << "interpolation operator not build";
            M_opDisp1dToNdconf->apply(M_solidModel->fieldDisplacementVect1dReduced(),
                                      *(M_fluidModel->meshDisplacementOnInterface() ) );
        }
        else
        {
            CHECK( M_opDisp1dToNdnonconf ) << "interpolation operator not build";
            M_opDisp1dToNdnonconf->apply(M_solidModel->fieldDisplacementVect1dReduced(),
                                         *(M_fluidModel->meshDisplacementOnInterface() ) );
        }
    }
    else std::cout << "[InterpolationFSI] : BUG " << std::endl;

    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertDisplacement", "finish",
                                               this->worldComm(),this->verboseAllProc());
}

//-----------------------------------------------------------------------------------//

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::transfertStress()
{
    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertStress", "start",
                                               this->worldComm(),this->verboseAllProc());

    M_fluidModel->updateNormalStressOnReferenceMesh(M_fluidModel->markersNameMovingBoundary());

    std::vector<int> saveActivities_stress = M_fluidModel->fieldNormalStressRefMesh().map().worldComm().activityOnWorld();
    if (M_fluidModel->worldComm().globalSize()>1  && !M_fluidModel->functionSpace()->hasEntriesForAllSpaces())
        M_fluidModel->fieldNormalStressRefMeshPtr()->map().worldComm().applyActivityOnlyOn(0/*VelocityWorld*/);

    if (M_solidModel->isStandardModel())
    {
        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opStress2dTo2dconf ) << "interpolation operator not build";
            M_opStress2dTo2dconf->apply( *(M_fluidModel->fieldNormalStressRefMeshPtr()), *(M_solidModel->fieldNormalStressFromFluidPtr()) );
        }
        else
        {
#if 1
            CHECK( M_opStress2dTo2dnonconf ) << "interpolation operator not build";
            M_opStress2dTo2dnonconf->apply( *(M_fluidModel->fieldNormalStressRefMeshPtr()), *(M_solidModel->fieldNormalStressFromFluidPtr()) );
#else
            //auto FluidPhysicalName = M_fluid->getMarkerNameFSI().front();
            auto mysubmesh = createSubmesh(this->fluidModel()->mesh(),markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()/*FluidPhysicalName*/));
            typedef Mesh<Simplex<1,1,2> > mymesh_type;
            typedef bases<Lagrange<1, Vectorial,Continuous,PointSetFekete> > mybasis_stress_type;
            typedef FunctionSpace<mymesh_type, mybasis_stress_type> myspace_stress_type;
            auto myXh = myspace_stress_type::New(mysubmesh);

            typedef bases<Lagrange<1, Vectorial,Discontinuous,PointSetFekete> > mybasis_disc_stress_type;
            typedef FunctionSpace<mymesh_type, mybasis_disc_stress_type> myspace_disc_stress_type;
            auto myXhdisc = myspace_disc_stress_type::New(mysubmesh);
            auto myudisc = myXhdisc->element();

            auto myopStressConv = opInterpolation(_domainSpace=M_fluidModel->fieldNormalStressRefMesh().functionSpace(),
                                                  _imageSpace=myXhdisc,
                                                  _range=elements(mysubmesh),
                                                  _type=InterpolationConforme(),
                                                  _backend=M_fluidModel->backend() );

            myopStressConv->apply(M_fluidModel->fieldNormalStressRefMesh(),myudisc);


            auto opProj = opProjection(_domainSpace=myXh,
                                       _imageSpace=myXh,
                                       //_backend=backend_type::build( this->vm(), "", worldcomm ),
                                       _type=Feel::L2);//L2;
            auto proj_beta = opProj->operator()(vf::trans(vf::idv(myudisc/*M_fluid->fieldNormalStressRefMesh()*/)));

            auto SolidPhysicalName = M_solidModel->markerNameFSI().front();
            auto myopStress2dTo2dnonconf = opInterpolation(_domainSpace=myXh,//M_fluid->fieldNormalStressRefMesh().functionSpace(),
                                                           _imageSpace=M_solidModel->getStress()->functionSpace(),
                                                           _range=markedfaces(M_solidModel->mesh(),SolidPhysicalName),
                                                           _type=InterpolationNonConforme(),
                                                           _backend=M_fluidModel->backend() );

            myopStress2dTo2dnonconf->apply(proj_beta,*(M_solidModel->getStress()));
#endif
        }




    }
    else if ( M_solidModel->is1dReducedModel() )
    {
        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opStress1dToNdconf )  << "interpolation operator not build";
            M_opStress1dToNdconf->apply(*(M_fluidModel->fieldNormalStressRefMeshPtr()), M_solidModel->fieldStressVect1dReduced() );
        }
        else
        {
            CHECK( M_opStress1dToNdnonconf )  << "interpolation operator not build";
            M_opStress1dToNdnonconf->apply(*(M_fluidModel->fieldNormalStressRefMeshPtr()), M_solidModel->fieldStressVect1dReduced() );
        }
        M_solidModel->updateInterfaceScalStressDispFromVectStress();
    }

    // revert initial activities
    if (M_fluidModel->worldComm().globalSize()>1  && !M_fluidModel->functionSpace()->hasEntriesForAllSpaces())
        M_fluidModel->fieldNormalStressRefMeshPtr()->map().worldComm().setIsActive(saveActivities_stress);

    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertStress", "finish",
                                               this->worldComm(),this->verboseAllProc());

}

//-----------------------------------------------------------------------------------//

#if 0
template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::transfertRobinNeumannInterfaceOperatorS2F()
{
    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertRobinNeumannInterfaceOperatorS2F", "start",
                                               this->worldComm(),this->verboseAllProc());

    if (!M_solidModel->isStandardModel()) return;

    auto fieldInterpolated = this->fluidModel()->functionSpaceVelocity()->elementPtr();//M_fluid->meshVelocity2().functionSpace()->elementPtr();
    auto fieldToTransfert = M_solidModel->functionSpaceDisplacement()->elementPtr(); //fieldVelocityPtr()->functionSpace()->elementPtr();
    CHECK( fieldToTransfert->map().nLocalDofWithGhost() == this->robinNeumannInterfaceOperator()->map().nLocalDofWithGhost() ) << "invalid compatibility size";
    *fieldToTransfert = *this->robinNeumannInterfaceOperator();
    if (M_interfaceFSIisConforme)
    {
        M_opVelocityBis2dTo2dconf/*auto opI*/ = opInterpolation(_domainSpace=this->solidModel()->functionSpaceDisplacement(),
                                   _imageSpace=this->fluidModel()->functionSpaceVelocity(),
                                   _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                   _type=InterpolationConforme(),
                                   _backend=this->fluidModel()->backend() );
        M_opVelocityBis2dTo2dconf/*opI*/->apply( *fieldToTransfert, *fieldInterpolated );
    }
    else
    {
        M_opVelocityBis2dTo2dnonconf/*auto opI*/ = opInterpolation(_domainSpace=this->solidModel()->functionSpaceDisplacement(),
                                   _imageSpace=this->fluidModel()->functionSpaceVelocity(),
                                   _range=markedfaces(this->fluidModel()->mesh(),this->fluidModel()->markersNameMovingBoundary()),
                                   _type=InterpolationNonConforme(),
                                   _backend=this->fluidModel()->backend() );
        M_opVelocityBis2dTo2dnonconf/*opI*/->apply( *fieldToTransfert, *fieldInterpolated );
    }

    M_fluid->setCouplingFSI_RNG_interfaceOperator( fieldInterpolated );
    M_fluid->setCouplingFSI_RNG_useInterfaceOperator( true );
    M_fluid->couplingFSI_RNG_updateForUse();

    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertRobinNeumannInterfaceOperatorS2F", "finish",
                                               this->worldComm(),this->verboseAllProc());
}
#endif

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::transfertVelocity(bool useExtrap)
{
    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertVelocity", "start",
                                               this->worldComm(),this->verboseAllProc());

    if (M_solidModel->isStandardModel())
    {
        typename solid_type::element_displacement_ptrtype fieldToTransfert;
        if ( !useExtrap )
        {
            fieldToTransfert = M_solidModel->fieldVelocityPtr();
        }
        else
        {
            fieldToTransfert = M_solidModel->fieldVelocity().functionSpace()->elementPtr();
            fieldToTransfert->add(  2.0, this->solidModel()->timeStepNewmark()->previousVelocity() );
            fieldToTransfert->add( -1.0, this->solidModel()->timeStepNewmark()->previousVelocity(1) );
        }

        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opVelocity2dTo2dconf ) << "interpolation operator not build";
            M_opVelocity2dTo2dconf->apply( *fieldToTransfert, M_fluidModel->meshVelocity2() );
        }
        else
        {
            CHECK( M_opVelocity2dTo2dnonconf ) << "interpolation operator not build";
            M_opVelocity2dTo2dnonconf->apply( *fieldToTransfert, M_fluidModel->meshVelocity2() );
        }
    }
    else if ( M_solidModel->is1dReducedModel() )
    {

        typename solid_type::element_vect_1dreduced_ptrtype fieldToTransfert;
        if( !useExtrap )
        {
            M_solidModel->updateInterfaceVelocityFrom1dVelocity();
            fieldToTransfert = M_solidModel->fieldVelocityVect1dReducedPtr();
        }
        else
        {
            if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertVelocity", "use extrapolation (1dReduced)",
                                                       this->worldComm(),this->verboseAllProc());
            //auto fieldExtrapolated = M_solidModel->fieldVelocityScal1dReduced().functionSpace()->elementPtr();
            auto fieldExtrapolated = this->solidModel()->timeStepNewmark1dReduced()->previousVelocity().functionSpace()->elementPtr();
            fieldExtrapolated->add(  2.0, this->solidModel()->timeStepNewmark1dReduced()->previousVelocity() );
            fieldExtrapolated->add( -1.0, this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(1) );
            fieldToTransfert = M_solidModel->extendVelocity1dReducedVectorial( *fieldExtrapolated );
        }

        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opVelocity1dToNdconf ) << "interpolation operator not build";
            M_opVelocity1dToNdconf->apply(*fieldToTransfert/*M_solid->fieldVelocityVect1dReduced()*/,
                                          M_fluidModel->meshVelocity2() );
        }
        else
        {
            CHECK( M_opVelocity1dToNdnonconf ) << "interpolation operator not build";
            M_opVelocity1dToNdnonconf->apply(*fieldToTransfert/*M_solid->fieldVelocityVect1dReduced()*/,
                                             M_fluidModel->meshVelocity2() );
        }
    }

    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertVelocity", "finish",
                                               this->worldComm(),this->verboseAllProc());
}

#if 0
template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::transfertRobinNeumannGeneralizedS2F( int iterationFSI )
{
    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertRobinNeumannGeneralizedS2F", "start",
                                               this->worldComm(),this->verboseAllProc());

    if ( this->solidModel()->isStandardModel())
    {
        auto fieldToTransfert = M_solidModel->fieldVelocity().functionSpace()->elementPtr();
        double dt = M_solidModel->timeStepNewmark()->timeStep();
        double gamma = M_solidModel->timeStepNewmark()->gamma();
        double beta = M_solidModel->timeStepNewmark()->beta();
        // time derivative acceleration in solid
        if ( true/*false*/ )
            fieldToTransfert->add( -1, M_solidModel->timeStepNewmark()->currentAcceleration());
        else
        {
            if ( iterationFSI == 0 )
            {
                fieldToTransfert->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                       this->solidModel()->timeStepNewmark()->previousVelocity(0) );
                for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                    fieldToTransfert->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                           this->solidModel()->timeStepNewmark()->previousVelocity(i+1) );
            }
            else
            {
                fieldToTransfert->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                       this->solidModel()->timeStepNewmark()->currentVelocity() );
                for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                    fieldToTransfert->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                           this->solidModel()->timeStepNewmark()->previousVelocity(i) );
            }
        }

        for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
            fieldToTransfert->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                   this->solidModel()->timeStepNewmark()->previousVelocity(i) );

        // transfer solid to fluid
        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opVelocity2dTo2dconf ) << "interpolation operator not build";
            M_opVelocity2dTo2dconf->apply( *fieldToTransfert, *this->couplingRNG_evalForm1() );
        }
        else
        {
            CHECK( M_opVelocity2dTo2dnonconf ) << "interpolation operator not build";
            M_opVelocity2dTo2dnonconf->apply( *fieldToTransfert, *this->couplingRNG_evalForm1() );
        }
        // time derivative acceleration in fluid
#if 0
        auto UPolyDeriv = this->fluidModel()->timeStepBDF()->polyDeriv();
        auto uPolyDeriv = UPolyDeriv.template element<0>();
        this->couplingRNG_evalForm1()->add( -1.0, uPolyDeriv );
        M_couplingRNG_coeffForm2 = this->fluidModel()->timeStepBDF()->polyDerivCoefficient(0);
#else
        M_couplingRNG_coeffForm2 = this->fluidModel()->timeStepBDF()->polyDerivCoefficient(0);
#endif

    }
    else if ( this->solidModel()->is1dReducedModel() )
    {
        typename solid_type::element_vect_1dreduced_ptrtype fieldToTransfert;
        auto fieldExtrapolated2 = this->solidModel()->timeStepNewmark1dReduced()->previousVelocity().functionSpace()->elementPtr();
        double dt = M_solidModel->timeStepNewmark1dReduced()->timeStep();
        double gamma = M_solidModel->timeStepNewmark1dReduced()->gamma();
        double beta = M_solidModel->timeStepNewmark1dReduced()->beta();
        double scaleTimeDisc = M_solidModel->mechanicalProperties()->cstRho()*M_solidModel->thickness1dReduced();
        // time derivative acceleration in solid
        if ( true )
            fieldExtrapolated2->add( -1, M_solidModel->timeStepNewmark1dReduced()->currentAcceleration());
        else
        {
            if ( iterationFSI == 0 )
            {
                fieldExtrapolated2->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                         this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(0) );
                for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                    fieldExtrapolated2->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                             this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(i+1) );
            }
            else
            {
                fieldExtrapolated2->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                         this->solidModel()->timeStepNewmark1dReduced()->currentVelocity() );
                for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                    fieldExtrapolated2->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                             this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(i) );
            }
        }
        // time derivative acceleration in fluid
        for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
            fieldExtrapolated2->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                     this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(i) );
        fieldExtrapolated2->scale( scaleTimeDisc );
        fieldToTransfert = M_solidModel->extendVelocity1dReducedVectorial( *fieldExtrapolated2 );
        // transfer solid to fluid
        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opVelocity1dToNdconf ) << "interpolation operator not build";
            M_opVelocity1dToNdconf->apply(*fieldToTransfert,
                                          *this->couplingRNG_evalForm1() );
        }
        else
        {
            CHECK( M_opVelocity1dToNdnonconf ) << "interpolation operator not build";
            M_opVelocity1dToNdnonconf->apply(*fieldToTransfert,
                                             *this->couplingRNG_evalForm1() );
        }
        // time derivative acceleration in fluid
        //auto UPolyDeriv = this->fluidModel()->timeStepBDF()->polyDeriv();
        //auto uPolyDeriv = UPolyDeriv.template element<0>();
        //this->couplingRNG_evalForm1()->add( -scaleTimeDisc, uPolyDeriv );

        M_couplingRNG_coeffForm2 = scaleTimeDisc*this->fluidModel()->timeStepBDF()->polyDerivCoefficient(0);
    }

    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertRobinNeumannGeneralizedS2F", "finish",
                                               this->worldComm(),this->verboseAllProc());
}
#elif 0
template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::transfertRobinNeumannGeneralizedS2F( int iterationFSI )
{
    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertRobinNeumannGeneralizedS2F", "start",
                                               this->worldComm(),this->verboseAllProc());
    bool useOriginalMethod = true;
    if (M_solidModel->isStandardModel())
    {
        auto fieldToTransfert = M_solidModel->fieldVelocity().functionSpace()->elementPtr();
        //typename solid_type::element_displacement_ptrtype fieldToTransfert;
        double dt = M_solidModel->timeStepNewmark()->timeStep();
        double gamma = M_solidModel->timeStepNewmark()->gamma();
        double beta = M_solidModel->timeStepNewmark()->beta();

#if 0
        if ( useOriginalMethod || (iterationFSI == 0) )
            fieldToTransfert->add( -1./(dt*gamma) , M_solidModel->timeStepNewmark()->previousVelocity() );
            //fieldToTransfert->add( (1./(dt*gamma))*( (gamma/beta)-1. ) -1./(beta*dt) , M_solidModel->timeStepNewmark()->previousVelocity() );
        //else
        //fieldToTransfert->add( (1./(dt*gamma))*( (gamma/beta)-1. ) -1./(beta*dt) , M_solidModel->timeStepNewmark()->currentVelocity() );

        if ( useOriginalMethod || (iterationFSI == 0) )
        {
            //fieldToTransfert->add( (1./gamma)*(gamma/(2*beta) - 1) - (1./(2*beta) -1), M_solidModel->timeStepNewmark()->previousAcceleration() );
            fieldToTransfert->add( -(1.-gamma)/gamma, M_solidModel->timeStepNewmark()->previousAcceleration() );
            fieldToTransfert->add( -1.0, M_solidModel->timeStepNewmark()->currentAcceleration() );
        }
        else
        {
            //fieldToTransfert->add( (1./gamma)*(gamma/(2*beta) - 1) - (1./(2*beta) -1) + 1.0, M_solidModel->timeStepNewmark()->currentAcceleration() );
            //fieldToTransfert->add( (1./gamma)*(gamma/(2*beta) - 1) - (1./(2*beta) -1) - 1.0, M_solidModel->timeStepNewmark()->currentAcceleration() );
            fieldToTransfert->add(-(1./(dt*gamma)), M_solidModel->timeStepNewmark()->currentVelocity() );
        }
#else
        fieldToTransfert->add( -1./(dt*gamma) , M_solidModel->timeStepNewmark()->previousVelocity() );
        fieldToTransfert->add( -(1.-gamma)/gamma, M_solidModel->timeStepNewmark()->previousAcceleration() );
        if ( true )
            fieldToTransfert->add( -1.0, M_solidModel->timeStepNewmark()->currentAcceleration() );
        else
        {
            if ( iterationFSI == 0 )
            {
                fieldToTransfert->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                       this->solidModel()->timeStepNewmark()->previousVelocity(0) );
                for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                    fieldToTransfert->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                           this->solidModel()->timeStepNewmark()->previousVelocity(i+1) );
            }
            else
            {
                fieldToTransfert->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                       this->solidModel()->timeStepNewmark()->currentVelocity() );
                for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                    fieldToTransfert->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                           this->solidModel()->timeStepNewmark()->previousVelocity(i) );
            }
        }
#endif

        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opVelocity2dTo2dconf ) << "interpolation operator not build";
            M_opVelocity2dTo2dconf->apply( *fieldToTransfert, *this->couplingRNG_evalForm1() );
        }
        else
        {
            CHECK( M_opVelocity2dTo2dnonconf ) << "interpolation operator not build";
            M_opVelocity2dTo2dnonconf->apply( *fieldToTransfert, *this->couplingRNG_evalForm1() );
        }

        M_couplingRNG_coeffForm2 = (1./(dt*gamma));
    }
    else if ( M_solidModel->is1dReducedModel() )
    {
        typename solid_type::element_vect_1dreduced_ptrtype fieldToTransfert;
        auto fieldExtrapolated2 = this->solidModel()->timeStepNewmark1dReduced()->previousVelocity().functionSpace()->elementPtr();
        double dt = M_solidModel->timeStepNewmark1dReduced()->timeStep();
        double gamma = M_solidModel->timeStepNewmark1dReduced()->gamma();
        double beta = M_solidModel->timeStepNewmark1dReduced()->beta();
        double scaleTimeDisc = M_solidModel->mechanicalProperties()->cstRho()*M_solidModel->thickness1dReduced();

#if 0
        if ( useOriginalMethod || (iterationFSI == 0)  )
            fieldExtrapolated2->add( (1./(dt*gamma))*( (gamma/beta)-1. ) -1./(beta*dt) , M_solidModel->timeStepNewmark1dReduced()->previousVelocity() );
        else
        {
            //fieldExtrapolated2->add( (1./(dt*gamma))*( (gamma/beta)-1. ) -1./(beta*dt) , M_solidModel->timeStepNewmark1dReduced()->currentVelocity() );
        }


        if ( useOriginalMethod || (iterationFSI == 0)  )
        {
            fieldExtrapolated2->add( (1./gamma)*(gamma/(2*beta) - 1) - (1./(2*beta) -1), M_solidModel->timeStepNewmark1dReduced()->previousAcceleration() );
            fieldExtrapolated2->add( -1.0, M_solidModel->timeStepNewmark1dReduced()->currentAcceleration() );
        }
        else
        {
            //fieldExtrapolated2->add( (1./gamma)*(gamma/(2*beta) - 1) - (1./(2*beta) -1) + 1.0, M_solidModel->timeStepNewmark1dReduced()->currentAcceleration() );
            fieldExtrapolated2->add(-(1./(dt*gamma)), M_solidModel->timeStepNewmark1dReduced()->currentVelocity() );
        }
        fieldExtrapolated2->scale( scaleTimeDisc );
#else
        fieldExtrapolated2->add( -1./(dt*gamma) , M_solidModel->timeStepNewmark1dReduced()->previousVelocity() );
        fieldExtrapolated2->add( -(1.-gamma)/gamma, M_solidModel->timeStepNewmark1dReduced()->previousAcceleration() );
        if ( true )
            fieldExtrapolated2->add( -1.0, M_solidModel->timeStepNewmark1dReduced()->currentAcceleration() );
        else
        {
            if ( iterationFSI == 0 )
            {
                fieldExtrapolated2->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                         this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(0) );
                for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                    fieldExtrapolated2->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                             this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(i+1) );
            }
            else
            {
                fieldExtrapolated2->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                         this->solidModel()->timeStepNewmark1dReduced()->currentVelocity() );
                for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                    fieldExtrapolated2->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                             this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(i) );
            }
        }
        fieldExtrapolated2->scale( scaleTimeDisc );
#endif
        fieldToTransfert = M_solidModel->extendVelocity1dReducedVectorial( *fieldExtrapolated2 );

        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opVelocity1dToNdconf ) << "interpolation operator not build";
            M_opVelocity1dToNdconf->apply(*fieldToTransfert,
                                          *this->couplingRNG_evalForm1() );
        }
        else
        {
            CHECK( M_opVelocity1dToNdnonconf ) << "interpolation operator not build";
            M_opVelocity1dToNdnonconf->apply(*fieldToTransfert,
                                             *this->couplingRNG_evalForm1() );
        }

        M_couplingRNG_coeffForm2 = scaleTimeDisc*(1./(dt*gamma));
    }

    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertRobinNeumannGeneralizedS2F", "finish",
                                               this->worldComm(),this->verboseAllProc());

}
#else
template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::transfertRobinNeumannGeneralizedS2F( int iterationFSI )
{
    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertRobinNeumannGeneralizedS2F", "start",
                                               this->worldComm(),this->verboseAllProc());

    if ( this->solidModel()->isStandardModel())
    {
        auto fieldToTransfert = M_solidModel->fieldVelocity().functionSpace()->elementPtr();
        double dt = M_solidModel->timeStepNewmark()->timeStep();
        double gamma = M_solidModel->timeStepNewmark()->gamma();
        double beta = M_solidModel->timeStepNewmark()->beta();
        // time derivative acceleration in solid (newmark)
        fieldToTransfert->add( -1, M_solidModel->timeStepNewmark()->currentAcceleration());
        // time derivative acceleration in solid (bdf)
        if ( iterationFSI == 0 )
        {
            fieldToTransfert->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                   this->solidModel()->timeStepNewmark()->previousVelocity(0) );
            for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                fieldToTransfert->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                       this->solidModel()->timeStepNewmark()->previousVelocity(i+1) );
        }
        else
        {
            fieldToTransfert->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                   this->solidModel()->timeStepNewmark()->currentVelocity() );
            for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                fieldToTransfert->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                       this->solidModel()->timeStepNewmark()->previousVelocity(i) );
        }

        // time derivative acceleration in fluid (bdf)
        for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
            fieldToTransfert->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                   this->solidModel()->timeStepNewmark()->previousVelocity(i) );
        // time derivative acceleration in fluid (newamrk)
        fieldToTransfert->add( -1./(dt*gamma) , this->solidModel()->timeStepNewmark()->previousVelocity() );
        fieldToTransfert->add( -(1.-gamma)/gamma, this->solidModel()->timeStepNewmark()->previousAcceleration() );

        fieldToTransfert->scale( 0.5 );

        // transfer solid to fluid
        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opVelocity2dTo2dconf ) << "interpolation operator not build";
            M_opVelocity2dTo2dconf->apply( *fieldToTransfert, *this->couplingRNG_evalForm1() );
        }
        else
        {
            CHECK( M_opVelocity2dTo2dnonconf ) << "interpolation operator not build";
            M_opVelocity2dTo2dnonconf->apply( *fieldToTransfert, *this->couplingRNG_evalForm1() );
        }
        // time derivative acceleration in fluid
#if 0
        auto UPolyDeriv = this->fluidModel()->timeStepBDF()->polyDeriv();
        auto uPolyDeriv = UPolyDeriv.template element<0>();
        this->couplingRNG_evalForm1()->add( -1.0, uPolyDeriv );
        M_couplingRNG_coeffForm2 = this->fluidModel()->timeStepBDF()->polyDerivCoefficient(0);
#else
        //M_couplingRNG_coeffForm2 = scaleTimeDisc*this->fluidModel()->timeStepBDF()->polyDerivCoefficient(0);
        M_couplingRNG_coeffForm2 = 0.5*(this->fluidModel()->timeStepBDF()->polyDerivCoefficient(0) + (1./(dt*gamma)));
#endif

    }
    else if ( this->solidModel()->is1dReducedModel() )
    {
        typename solid_type::element_vect_1dreduced_ptrtype fieldToTransfert;
        auto fieldExtrapolated2 = this->solidModel()->timeStepNewmark1dReduced()->previousVelocity().functionSpace()->elementPtr();
        double dt = M_solidModel->timeStepNewmark1dReduced()->timeStep();
        double gamma = M_solidModel->timeStepNewmark1dReduced()->gamma();
        double beta = M_solidModel->timeStepNewmark1dReduced()->beta();
        double scaleTimeDisc = M_solidModel->mechanicalProperties()->cstRho()*M_solidModel->thickness1dReduced();
        // time derivative acceleration in solid (newmark)
        fieldExtrapolated2->add( -1, M_solidModel->timeStepNewmark1dReduced()->currentAcceleration());
        // time derivative acceleration in solid (bdf)
        if ( iterationFSI == 0 )
        {
            fieldExtrapolated2->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                     this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(0) );
            for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                fieldExtrapolated2->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                         this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(i+1) );
        }
        else
        {
            fieldExtrapolated2->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( 0 ),
                                     this->solidModel()->timeStepNewmark1dReduced()->currentVelocity() );
            for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
                fieldExtrapolated2->add( this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                         this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(i) );
        }

        // time derivative acceleration in fluid (bdf)
        for ( uint8_type i = 0; i < this->fluidModel()->timeStepBDF()->timeOrder(); ++i )
            fieldExtrapolated2->add( -this->fluidModel()->timeStepBDF()->polyDerivCoefficient( i+1 ),
                                     this->solidModel()->timeStepNewmark1dReduced()->previousVelocity(i) );
        // time derivative acceleration in fluid (newamrk)
        fieldExtrapolated2->add( -1./(dt*gamma) , M_solidModel->timeStepNewmark1dReduced()->previousVelocity() );
        fieldExtrapolated2->add( -(1.-gamma)/gamma, M_solidModel->timeStepNewmark1dReduced()->previousAcceleration() );

        fieldExtrapolated2->scale( 0.5*scaleTimeDisc );
        fieldToTransfert = M_solidModel->extendVelocity1dReducedVectorial( *fieldExtrapolated2 );
        // transfer solid to fluid
        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opVelocity1dToNdconf ) << "interpolation operator not build";
            M_opVelocity1dToNdconf->apply(*fieldToTransfert,
                                          *this->couplingRNG_evalForm1() );
        }
        else
        {
            CHECK( M_opVelocity1dToNdnonconf ) << "interpolation operator not build";
            M_opVelocity1dToNdnonconf->apply(*fieldToTransfert,
                                             *this->couplingRNG_evalForm1() );
        }
        // time derivative acceleration in fluid
        //auto UPolyDeriv = this->fluidModel()->timeStepBDF()->polyDeriv();
        //auto uPolyDeriv = UPolyDeriv.template element<0>();
        //this->couplingRNG_evalForm1()->add( -scaleTimeDisc, uPolyDeriv );

        M_couplingRNG_coeffForm2 = scaleTimeDisc*(0.5*this->fluidModel()->timeStepBDF()->polyDerivCoefficient(0)+0.5*(1./(dt*gamma)));
    }

    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertRobinNeumannGeneralizedS2F", "finish",
                                               this->worldComm(),this->verboseAllProc());
}
#endif
//-----------------------------------------------------------------------------------//

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::transfertStressS2F()
{
    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertStressS2F", "start",
                                               this->worldComm(),this->verboseAllProc());

    this->solidModel()->updateNormalStressFromStruct();
    CHECK( M_opStress2dTo2dconfS2F ) << "interpolation operator not build";
    M_opStress2dTo2dconfS2F->apply(*this->solidModel()->fieldNormalStressFromStructPtr(),
                                   *this->fluidModel()->normalStressFromStruct());

    if (this->verbose()) Feel::FeelModels::Log("InterpolationFSI","transfertStressS2F", "finish",
                                               this->worldComm(),this->verboseAllProc());
}

template< class FluidType, class SolidType >
void
FSI<FluidType,SolidType>::transfertVelocityF2S( int iterationFSI, bool _useExtrapolation )
{
    bool useExtrapolation = ( iterationFSI == 0) && _useExtrapolation && (this->fluidModel()->timeStepBDF()->iteration() > 2);
    if ( useExtrapolation )
    {
        if ( true )
        {
            // bdf extrapolation
            auto solExtrap = this->fluidModel()->timeStepBDF()->poly();
            auto velExtrap = solExtrap.template element<0>();
            if (M_interfaceFSIisConforme)
            {
                CHECK( M_opVelocity2dTo2dconfF2S ) << "interpolation operator not build";
                M_opVelocity2dTo2dconfF2S->apply( velExtrap,*this->solidModel()->fieldVelocityInterfaceFromFluidPtr() );
            }
            else
            {
                CHECK(false) << "TODO\n";
            }
        }
        else
        {
            // extrap in Explicit strategies for incompressible fluid-structure interaction problems: Nitche ....
#if 0
            auto velExtrap = this->fluidModel()->functionSpace()->template functionSpace<0>()->element();
            velExtrap.add(  2.0, this->fluidModel()->getSolution()->template element<0>() );
            velExtrap.add( -1.0, this->fluidModel()->timeStepBDF()->unknown(0).template element<0>() );
#else
            auto velExtrap = this->fluidModel()->functionSpace()->template functionSpace<0>()->element();
            velExtrap.add(  2.0, this->fluidModel()->timeStepBDF()->unknown(0).template element<0>() );
            velExtrap.add( -1.0, this->fluidModel()->timeStepBDF()->unknown(1).template element<0>() );
#endif
            if (M_interfaceFSIisConforme)
            {
                CHECK( M_opVelocity2dTo2dconfF2S ) << "interpolation operator not build";
                M_opVelocity2dTo2dconfF2S->apply( velExtrap,*this->solidModel()->fieldVelocityInterfaceFromFluidPtr() );
            }
            else
            {
                CHECK(false) << "TODO\n";
            }
        }
    }
    else // no extrap : take current solution
    {
        if (M_interfaceFSIisConforme)
        {
            CHECK( M_opVelocity2dTo2dconfF2S ) << "interpolation operator not build";
#if 0
            M_opVelocity2dTo2dconfF2S->apply( this->fluidModel()->timeStepBDF()->unknown(0).template element<0>(),
                                              *this->solidModel()->fieldVelocityInterfaceFromFluidPtr() );
#else
            M_opVelocity2dTo2dconfF2S->apply( this->fluidModel()->fieldVelocity(),
                                              *this->solidModel()->fieldVelocityInterfaceFromFluidPtr() );
#endif
        }
        else
        {
            CHECK(false) << "TODO\n";
        }
    }
}




} // namespace FeelModels
} // namespace Feel
