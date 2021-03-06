//
//  Energy.h
//  Gauss
//
//  Created by David Levin on 3/13/17.
//
//

#ifndef Energy_h
#define Energy_h

namespace Gauss {
    namespace FEM {

        /////// Kinetic Energy Terms //////
        template<typename DataType, typename ShapeFunction>
        class EnergyKineticNone : public virtual ShapeFunction {

        };

        //Non-lumped kinetic energy
        template<typename DataType, typename ShapeFunction>
        class EnergyKineticNonLumped : public virtual ShapeFunction {
        public:
            template<typename QDOFList, typename QDotDOFList>
            EnergyKineticNonLumped(Eigen::MatrixXd &V, Eigen::MatrixXi &F, QDOFList &qDOFList, QDotDOFList &qDotDOFList) : ShapeFunction(V, F, qDOFList, qDotDOFList) {
                m_rho = 1000.0;
            }

            inline void setDensity(double density) { m_rho = density; }

            inline double getDensity() { return m_rho; }

            inline DataType getValue(double *x, const State<DataType> &state) {
                auto v = ShapeFunction::J(x, state)*ShapeFunction::qDot(state);
                return 0.5*m_rho*v.transpose()*v;
            }

            //infinitessimal gradients and hessians
            template<typename Vector>
            inline void getGradient(Vector &f, double *x, const State<DataType> &state) {
                //do Nothing (I don't think I use this for anything really)
            }


            template<typename Matrix>
            inline void getHessian(Matrix &H, double *x, const State<DataType> &state) {

                //compute mass matrix at point X = m_rho*J(x)^T*J(x)
                typename ShapeFunction::MatrixJ M = ShapeFunction::J(x, state);

                H = m_rho*M.transpose()*M;
            }

        protected:
            double m_rho;

        private:

        };

        /////// Potential Energy Terms //////
        template<typename DataType, typename ShapeFunction>
        class EnergyPotentialNone : public virtual ShapeFunction {
        public:
            template<typename DOFList>
            EnergyPotentialNone(Eigen::MatrixXd &V, Eigen::MatrixXi &F, DOFList &dofList) : ShapeFunction(V,F, dofList) { }
        };

        template<typename DataType, typename ShapeFunction>
        class EnergyLinearElasticity : public ShapeFunction {
        public:
            template<typename QDOFList, typename QDotDOFList>
            EnergyLinearElasticity(Eigen::MatrixXd &V, Eigen::MatrixXi &F, QDOFList &qDOFList, QDotDOFList &qDotDOFList) : ShapeFunction(V, F, qDOFList, qDotDOFList) {
                setParameters(1e6, 0.45);

            }

            inline void setParameters(double youngsModulus, double poissonsRatio) {
                m_E = youngsModulus;
                m_mu = poissonsRatio;

                m_C.setZero();
                m_C(0,0) = 1.0-m_mu;
                m_C(0,1) = m_mu;
                m_C(0,2) = m_mu;
                m_C(1,0) = m_mu;
                m_C(1,1) = 1.0-m_mu;
                m_C(1,2) = m_mu;
                m_C(2,0) = m_mu;
                m_C(2,1) = m_mu;
                m_C(2,2) = 1.0-m_mu;
                m_C(3,3) = 0.5*(1.0-2.0*m_mu);
                m_C(4,4) = 0.5*(1.0-2.0*m_mu);
                m_C(5,5) = 0.5*(1.0-2.0*m_mu);
                m_C *= (m_E/((1.0+m_mu)*(1.0-2.0*m_mu)));
            }

            inline DataType getValue(double *x, const State<DataType> &state) {

                auto q = ShapeFunction::q(state);

                return -0.5*q.transpose()*B(this, x, state).transpose()*m_C*B(this, x, state)*q;
            }

            template<typename Vector>
            inline void getGradient(Vector &f, double *x, const State<DataType> &state) {

                /*//returning the force which is really the negative gradient*/
                f = -B(this, x, state).transpose()*m_C*B(this, x, state)*ShapeFunction::q(state);

            }

            template<typename Matrix>
            inline void getHessian(Matrix &H, double *x, const State<DataType> &state) {
                H = -B(this, x, state).transpose()*m_C*B(this, x, state);

            }

            template<typename Matrix>
            inline void getCauchyStress(Matrix &S, double *x, State<DataType> &state) {
                
                Eigen::Matrix<DataType, 6, 1> s = m_C*B(this,x,state)*ShapeFunction::q(state);
                
                S(0,0) = s(0); S(0,1) = s(5); S(0,2) = s(4);
                S(1,0) = s(5); S(1,1) = s(1); S(1,2) = s(3);
                S(2,0) = s(4); S(2,1) = s(3); S(2,2) = s(2);
                
            }

            inline const DataType & getE() const { return m_E; }
            inline const DataType & getMu() const { return m_mu; }

        protected:
            DataType m_E, m_mu;
            Eigen::Matrix66x<DataType> m_C;

        private:

        };

        //////// Body Force Terms /////////
        template<typename DataType, typename ShapeFunction>
        class BodyForceNone : public virtual ShapeFunction {
        public:
             template<typename QDOFList, typename QDotDOFList, typename ...Params>
            BodyForceNone(Eigen::MatrixXd &V, Eigen::MatrixXi &F, QDOFList &qDOFList, QDotDOFList &qDotDOFList, Params ...params) : ShapeFunction(V,F, qDOFList, qDotDOFList) { }
        };

        template<typename DataType, typename ShapeFunction>
        class BodyForceGravity : public virtual ShapeFunction {
        public:
            template<typename QDOFList, typename QDotDOFList>
            BodyForceGravity(Eigen::MatrixXd &V, Eigen::MatrixXi &F, QDOFList &qDOFList, QDotDOFList &qDotDOFList) : ShapeFunction(V,F, qDOFList, qDotDOFList)
            {
                m_rho = 1;
                m_g << 0.0,-9.8,0.0;
            }

            inline void setBodyForceDensity(double rho) {
                m_rho = rho;
            }

            void setGravity(Eigen::Vector3x<DataType> &g) {
                m_g = g;
            }

            inline  DataType getValue(double *x, const State<DataType> &state) {

                return  -ShapeFunction::q(state).transpose()*ShapeFunction::J(x,state).transpose()*m_rho*m_g;
            }

            template<typename Vector>
            inline void getGradient(Vector &f, double *x, const State<DataType> &state) {

                //assuming force does same rate of work so generalized force = J'T*f
                f = ShapeFunction::J(x,state).transpose()*m_rho*m_g;
            }

            template<typename Matrix>
            inline void getHessian(Matrix &H, double *x, const State<DataType> &state) {

            }

        protected:
            Eigen::Vector3x<DataType> m_g;
            double m_rho;
        };

    }
}


#endif /* Energy_h */
