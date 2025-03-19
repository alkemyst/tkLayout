class IrradiationStats {
 public:
   double min=0;
   double max=0;
   double avg=0;

   // Overload multiplication operator to multiply by a scalar
   IrradiationStats operator*(double scalar) const {
      IrradiationStats result;
      result.min = min * scalar;
      result.max = max * scalar;
      result.avg = avg * scalar;
      return result;
   }

   // Overload multiplication assignment operator (*=)
   IrradiationStats& operator*=(double scalar) {
      min *= scalar;
      max *= scalar;
      avg *= scalar;
      return *this;
   }
};
