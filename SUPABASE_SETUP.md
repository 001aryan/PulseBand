# Supabase Setup for PulseBand

## Quick Start

1. **Create Supabase account**
   - Go to https://supabase.com
   - Sign up with GitHub
   - Create a new project (choose region closest to you)

2. **Get API Keys**
   - In Supabase dashboard, go to **Settings → API**
   - Copy your:
     - `Project URL` (e.g., `https://xxxxx.supabase.co`)
     - `anon` public key
     - `service_role` secret key (keep this private!)

3. **Run the SQL Schema**
  - In Supabase, go to **SQL Editor**
  - Click **New Query**
  - Open [supabase.sql](supabase.sql) in this workspace
  - Copy its full contents into the editor
  - Click **Run**

4. **Enable RLS (Row Level Security)**
   - Go to **Authentication → Policies**
   - Ensure RLS is enabled (default)

---

## SQL Schema

The live schema is stored in [supabase.sql](supabase.sql). Paste the full file into Supabase SQL Editor and run it once.

---

## Environment Variables

After running the schema, you'll have:

```bash
# .env file for your backend
SUPABASE_URL=https://xxxxx.supabase.co
SUPABASE_ANON_KEY=eyJxx...
SUPABASE_SERVICE_KEY=eyJxx...  # Keep this secret!
```

---

## Verifying the Schema

In Supabase, go to **Table Editor** and verify:
- ✅ `users` table exists
- ✅ `readings` table exists
- ✅ `emergency_contacts` table exists
- ✅ `emergency_alerts` table exists

---

## Next Steps

1. Save the API keys to a `.env` file in `backend/`
2. I'll refactor `backend/server.js` to use Supabase
3. Create Render deployment config
4. Deploy!

---

## Troubleshooting

**"RLS policy error"?**
- Make sure you're logged in as the project owner
- RLS policies are permissive for personal use

**"Foreign key constraint failed"?**
- First insert into `users` table, then use that user_id for readings

**Need test data?**
```sql
-- Insert a test user
INSERT INTO users (email, name) VALUES ('you@example.com', 'Test User')
RETURNING id;

-- Copy the returned UUID, then:
INSERT INTO readings (user_id, bpm, signal, threshold)
VALUES ('YOUR-UUID', 75, 2000, 1800);
```

---

## Costs

Supabase free tier includes:
- ✅ 500 MB storage
- ✅ 2M API calls/month
- ✅ 50MB monthly bandwidth
- **Perfect for a personal project**

Ready? Let me know once you've:
1. Created Supabase project
2. Copied the schema into SQL Editor
3. Got your API keys

Then I'll create the refactored backend!
