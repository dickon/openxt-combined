--
-- Copyright (c) 2012 Citrix Systems, Inc.
-- 
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.
-- 
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
-- 
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--

{-# LANGUAGE GeneralizedNewtypeDeriving #-}

module Rpc ( Rpc
           , rpc
           , module Rpc.Core ) where

import Control.Applicative
import Control.Monad.Error
import Control.Monad.Trans
import qualified Control.Exception as E
import Rpc.Core
import Tools.FreezeIOM

import Errors

newtype Rpc a = Rpc (RpcM FuseChatError a)
    deriving (Functor, Applicative, Monad, MonadError FuseChatError, MonadRpc FuseChatError, FreezeIOM RpcContext (Either FuseChatError))

rpc ctx (Rpc f) = runRpcM f ctx

instance MonadIO Rpc where
    liftIO act =  Rpc $
        do status <- liftIO $ E.try act
           -- translate io errors into monadic version
           tunnel status
        where
          tunnel :: Either E.SomeException a -> RpcM FuseChatError a
          tunnel (Right v)  = return v
          tunnel (Left err) = failIO $ show err
