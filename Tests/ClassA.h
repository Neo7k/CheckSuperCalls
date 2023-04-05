#pragma once

namespace alpha
{
	class B;
	
	namespace beta
	{
		class A
		{
		public:
		
			[[using gnu : const, always_inline, hot]] [[nodiscard, csc::call_super]]
			virtual int Func_Super() const = 0;
			
			// [[nodiscard, csc::call_super]]
			virtual void Func_NoSuper() const throw();
			
			[[csc::call_super]]
			void Func_NonVirtual();
			
			//***********Some comment*********//
			/*[[csc::call_super]] void Func_CommentedOut();
			*/
			[[nodiscard]][[csc::call_super]]
			virtual int Func_HeaderDefined()
			{
				//...
				return 0;
			}
			
			virtual const B& Func_WrongContext(bool call_super = false) noexcept(false);
			auto call_super() -> bool {
				// wrong context
				return false;
			}
			
			struct Nested
			{
				[[csc::call_super]]
				virtual void Func_Nested();
			};
			
			[[csc::call_super]]
			virtual std::vector<int> Func_Super_II();
		};
	}
	
}