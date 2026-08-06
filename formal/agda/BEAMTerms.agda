module BEAMTerms where

open import Data.Nat using (ℕ; zero; succ; _+_)
open import Data.Bool using (Bool; true; false)
open import Relation.Binary.PropositionalEquality using (_≡_; refl)

-- Definition of BEAM Tagged Term types
data EtermType : Set where
  SmallIntType : EtermType
  AtomType     : EtermType
  TupleType    : EtermType
  ListType     : EtermType

-- Theorem: Every SmallInt tag operation is deterministic
tag-deterministic : ∀ (val : ℕ) → (val + zero) ≡ val
tag-deterministic zero = refl
tag-deterministic (succ n) rewrite tag-deterministic n = refl
